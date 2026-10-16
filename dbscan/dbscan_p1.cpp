// DBSCAN ingenuo para detectar ruido - versión paralela P1: "toda la matriz indivisible".
//
// Estrategia: la matriz de puntos NO se divide. Todos los hilos ven los n puntos completos y lo que
// se reparte son las iteraciones del ciclo exterior (un punto i por iteración): cada hilo calcula la
// etiqueta de sus puntos comparándolos contra los n puntos, exactamente como la versión serial.
// Es la paralelización más directa del algoritmo: mismo trabajo O(n²), dividido entre los hilos.
//
// Uso:  ./dbscan_p1 input.csv output.csv threads
//       ./dbscan_p1                               (sin argumentos: datos de 4000 puntos y todos los cores)
#include <omp.h>
#include "common.hpp"

int main(int argc, char* argv[]) {
    // por defecto, omp_get_max_threads() = número de cores virtuales de la máquina
    vector<string> args = getArgs(argc, argv,
                                  {DEFAULT_INPUT, DEFAULT_OUTPUT, to_string(omp_get_max_threads())},
                                  "input.csv output.csv threads");
    int threads = stoi(args[2]);
    omp_set_num_threads(threads);   // número de hilos de todas las regiones paralelas siguientes

    int n = 0;
    double** points = readCSV(args[0].c_str(), n);
    bool* isCore = new bool[n];   // resultado del paso 1: ¿el punto es core de primer orden?
    int* labels = new int[n];     // etiqueta final de cada punto (NOISE, CORE1 o CORE2)

    // igual que en la serial: se mide sólo el algoritmo, sin leer ni escribir el csv
    double start = omp_get_wtime();

    // Paso 1 en paralelo: las iteraciones i se reparten entre los hilos.
    // ¿Por qué no hay condiciones de carrera?
    //   - points sólo se LEE (todos los hilos pueden leerlo a la vez sin problema);
    //   - la iteración i sólo ESCRIBE isCore[i] y labels[i], posiciones que ningún otro hilo toca;
    //   - neighbors se declara dentro del ciclo, así que cada hilo tiene su propia copia (privada).
    // Por eso no se necesitan critical, atomic ni locks, que serializarían el trabajo.
    //
    // schedule(dynamic, 64): cada hilo toma bloques de 64 puntos consecutivos y pide otro al terminar.
    //   - dynamic: el M3 tiene cores rápidos (performance) y lentos (efficiency); con static los hilos
    //     de los cores lentos se quedarían atrás y los rápidos esperarían en la barrera. Con dynamic,
    //     el que termina antes toma más trabajo.
    //   - 64: pedir un bloque tiene un costo; bloques de 64 lo amortizan. Además 64 posiciones
    //     consecutivas de isCore (bool, 1 byte) llenan una línea de caché de 64 bytes, así que dos
    //     hilos casi nunca escriben en la misma línea (se evita el false sharing).
    #pragma omp parallel for schedule(dynamic, 64)
    for (int i = 0; i < n; i++) {
        int neighbors = 0;   // privada: declarada dentro del ciclo paralelo
        for (int j = 0; j < n; j++)
            if (dist2(points[i], points[j]) <= EPS2)
                neighbors++;
        isCore[i] = (neighbors >= MIN_SAMPLES);
        labels[i] = isCore[i] ? CORE1 : NOISE;
    }
    // Barrera implícita al final del parallel for: ningún hilo empieza el paso 2 hasta que TODOS
    // terminan el paso 1. Es indispensable, porque el paso 2 lee isCore[j] de puntos de otros hilos
    // y necesita que ya tengan su valor final.

    // Paso 2 en paralelo, mismo razonamiento: isCore sólo se lee y labels[i] sólo lo escribe la
    // iteración i. Aquí la carga SÍ es desigual (los cores se saltan de inmediato, un ruido aislado
    // recorre los n puntos sin encontrar core y otro sale con el break al primer core), otra razón
    // para usar dynamic.
    #pragma omp parallel for schedule(dynamic, 64)
    for (int i = 0; i < n; i++) {
        if (isCore[i])
            continue;
        for (int j = 0; j < n; j++) {
            if (isCore[j] && dist2(points[i], points[j]) <= EPS2) {
                labels[i] = CORE2;
                break;   // break del ciclo interior (serial dentro del hilo): sí está permitido
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
