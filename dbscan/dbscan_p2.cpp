// DBSCAN ingenuo para detectar ruido - versión paralela P2: "dividiendo la matriz por k".
//
// Estrategia: el espacio se divide en una cuadrícula de k bloques (rows x cols). Cada bloque es una
// tarea independiente y los bloques se reparten dinámicamente entre los hilos.
//
// ¿Cómo se unen los resultados para mantener la consistencia? (pregunta del enunciado)
// No hace falta un paso de "unión", porque se cumplen tres reglas:
//   1. Dueño único: cada punto pertenece (owned) a exactamente UN bloque y sólo ese bloque escribe
//      su etiqueta. Todos escriben directo en los arreglos globales isCore/labels, en posiciones
//      distintas, así que esos arreglos YA son el resultado unido y no hay condiciones de carrera.
//   2. Halo: un punto cerca de la frontera tiene vecinos en el bloque de al lado. Por eso cada bloque
//      además LEE (sin escribir) los puntos de los bloques vecinos que están a <= EPS de su borde.
//      Así cada punto ve a todos sus vecinos y recibe la misma etiqueta que en la versión serial.
//   3. Barrera entre pasos: el paso 2 de un bloque lee isCore de puntos del halo, que son de OTROS
//      bloques; la barrera garantiza que esos valores ya son finales cuando se leen.
//
// Ventaja sobre P1: cada punto se compara sólo contra los candidatos de su bloque (~n/k puntos más
// el halo) y no contra los n puntos, así que se hace mucho menos trabajo que en la serial.
//
// Uso:  ./dbscan_p2 input.csv output.csv threads k
//       ./dbscan_p2                                 (sin argumentos: 4000 puntos, todos los cores, k=16)
#include <omp.h>
#include <vector>
#include "common.hpp"

int main(int argc, char* argv[]) {
    vector<string> args = getArgs(argc, argv,
                                  {DEFAULT_INPUT, DEFAULT_OUTPUT, to_string(omp_get_max_threads()),
                                   to_string(DEFAULT_K)},
                                  "input.csv output.csv threads k");
    int threads = stoi(args[2]);
    int k = stoi(args[3]);
    omp_set_num_threads(threads);

    int n = 0;
    double** points = readCSV(args[0].c_str(), n);
    bool* isCore = new bool[n];   // resultado del paso 1: ¿el punto es core de primer orden?
    int* labels = new int[n];     // etiqueta final de cada punto (NOISE, CORE1 o CORE2)

    // Se mide todo lo que hace P2 además de leer/escribir: también construir los bloques, porque
    // es trabajo extra que la serial y P1 no hacen y sería injusto dejarlo fuera.
    double start = omp_get_wtime();

    // Cuadrícula de rows x cols = k bloques, lo más cuadrada posible:
    // k=2 -> 1x2, 4 -> 2x2, 8 -> 2x4, 16 -> 4x4, 32 -> 4x8, 64 -> 8x8
    // Decisión: bloques casi cuadrados tienen menos perímetro para la misma área, y el halo crece
    // con el perímetro; tiras muy delgadas tendrían proporcionalmente mucho más halo.
    int rows = 1;
    for (int r = 1; r * r <= k; r++)
        if (k % r == 0)
            rows = r;   // el divisor de k más grande que no pasa de sqrt(k)
    int cols = k / rows;

    // Caja que contiene a todos los puntos (bounding box). Se recorre una vez, O(n): es muy barato
    // comparado con el O(n²) de los pasos 1 y 2, por eso se deja serial.
    double minX = points[0][0], maxX = points[0][0];
    double minY = points[0][1], maxY = points[0][1];
    for (int i = 1; i < n; i++) {
        if (points[i][0] < minX) minX = points[i][0];
        if (points[i][0] > maxX) maxX = points[i][0];
        if (points[i][1] < minY) minY = points[i][1];
        if (points[i][1] > maxY) maxY = points[i][1];
    }
    double width = (maxX - minX) / cols;    // ancho de un bloque
    double height = (maxY - minY) / rows;   // alto de un bloque

    // owned[b]: índices de los puntos que están DENTRO del bloque b (sólo b escribe sus etiquetas).
    // Cada punto cae en exactamente un bloque: regla 1, dueño único.
    // Decisión: se guardan índices (int) y no copias de los puntos, así la salida conserva el orden
    // original y los resultados se escriben directo en isCore[i] / labels[i].
    vector<vector<int>> owned(k);
    for (int i = 0; i < n; i++) {
        int c = (int)((points[i][0] - minX) / width);    // columna del bloque
        int r = (int)((points[i][1] - minY) / height);   // fila del bloque
        if (c == cols) c = cols - 1;   // el punto con x = maxX daría c = cols: va al último bloque
        if (r == rows) r = rows - 1;
        owned[r * cols + c].push_back(i);
    }

    // candidates[b]: puntos que pueden ser vecinos de los puntos del bloque b, es decir, los puntos
    // dentro del bloque b agrandado EPS por cada lado (bloque + halo). Regla 2.
    // El 1e-9 extra protege contra errores de redondeo en las fronteras: sin él, un vecino a
    // distancia exactamente EPS de un punto en el borde podría quedar fuera del halo por 1e-16.
    // Paralelo por bloques: cada hilo sólo hace push_back en candidates[b] de SU bloque b, y el
    // vector exterior ya tiene tamaño k, así que no hay carrera.
    vector<vector<int>> candidates(k);
    double margin = EPS + 1e-9;
    #pragma omp parallel for schedule(dynamic, 1)
    for (int b = 0; b < k; b++) {
        int r = b / cols;
        int c = b % cols;
        double x0 = minX + c * width - margin;
        double x1 = minX + (c + 1) * width + margin;
        double y0 = minY + r * height - margin;
        double y1 = minY + (r + 1) * height + margin;
        for (int i = 0; i < n; i++)
            if (points[i][0] >= x0 && points[i][0] <= x1 &&
                points[i][1] >= y0 && points[i][1] <= y1)
                candidates[b].push_back(i);
    }

    // Paso 1: cada bloque etiqueta SUS puntos comparándolos sólo contra sus candidatos.
    // El bloque b sólo escribe isCore/labels de puntos que le pertenecen: no hay carreras.
    //
    // schedule(dynamic, 1): se reparte bloque por bloque. Los bloques tienen cargas MUY distintas
    // (un bloque sobre un cluster denso tiene miles de puntos y otro en una esquina casi ninguno);
    // con static a un hilo le podrían tocar todos los bloques pesados. Con dynamic, el hilo que
    // termina toma el siguiente bloque. Chunk de 1 porque hay pocas tareas (k <= 64), así que el
    // costo de repartirlas es despreciable.
    // Consecuencia que se ve en las gráficas: si k es menor que el número de hilos, sobran hilos sin
    // trabajo (con k = 2 sólo trabajan 2 hilos, aunque se pidan 16).
    #pragma omp parallel for schedule(dynamic, 1)
    for (int b = 0; b < k; b++) {
        for (int a = 0; a < (int)owned[b].size(); a++) {
            int i = owned[b][a];
            int neighbors = 0;
            for (int c = 0; c < (int)candidates[b].size(); c++) {
                int j = candidates[b][c];
                if (dist2(points[i], points[j]) <= EPS2)
                    neighbors++;
            }
            isCore[i] = (neighbors >= MIN_SAMPLES);
            labels[i] = isCore[i] ? CORE1 : NOISE;
        }
    }
    // Barrera implícita (regla 3): el paso 2 empieza sólo cuando isCore está completo en TODOS los
    // bloques, así los puntos del halo de cada bloque ya tienen su valor final.

    // Paso 2: los puntos de ruido del bloque b buscan un core de primer orden entre los candidatos
    // de b. Todo vecino a <= EPS está en candidates[b] gracias al halo, así que no se pierde ninguno.
    #pragma omp parallel for schedule(dynamic, 1)
    for (int b = 0; b < k; b++) {
        for (int a = 0; a < (int)owned[b].size(); a++) {
            int i = owned[b][a];
            if (isCore[i])
                continue;
            for (int c = 0; c < (int)candidates[b].size(); c++) {
                int j = candidates[b][c];
                if (isCore[j] && dist2(points[i], points[j]) <= EPS2) {
                    labels[i] = CORE2;
                    break;   // basta con un core de primer orden cercano
                }
            }
        }
    }

    double end = omp_get_wtime();
    cout << end - start << "\n";   // sólo el tiempo, para que run_experiments.py lo lea

    writeCSV(args[1].c_str(), points, labels, n);

    delete[] isCore;
    delete[] labels;
    freePoints(points, n);
    return 0;
}
