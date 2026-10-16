// DBSCAN ingenuo para detectar ruido - versión SERIAL.
// Es la referencia del proyecto: los speedups de P1 y P2 se calculan contra este tiempo, y sus
// resultados deben ser idénticos a los de esta versión.
//
// Uso:  ./dbscan_serial input.csv output.csv
//       ./dbscan_serial                        (sin argumentos: data/4000_data.csv -> data/4000_results.csv)
//
// omp.h se incluye sólo para medir el tiempo con omp_get_wtime(), el mismo reloj que usan P1 y P2,
// para que los tiempos de las tres versiones sean comparables. Aquí no hay ninguna región paralela.
#include <omp.h>
#include "common.hpp"

int main(int argc, char* argv[]) {
    vector<string> args = getArgs(argc, argv, {DEFAULT_INPUT, DEFAULT_OUTPUT},
                                  "input.csv output.csv");

    int n = 0;
    double** points = readCSV(args[0].c_str(), n);
    bool* isCore = new bool[n];   // resultado del paso 1: ¿el punto es core de primer orden?
    int* labels = new int[n];     // etiqueta final de cada punto (NOISE, CORE1 o CORE2)

    // Decisión: se mide SÓLO el algoritmo (pasos 1 y 2). Leer y escribir el csv no se mide porque
    // es igual en las tres versiones y no se paraleliza; incluirlo escondería el speedup real.
    double start = omp_get_wtime();

    // Paso 1: un punto es core de primer orden si tiene al menos MIN_SAMPLES puntos (él incluido)
    // a distancia <= EPS; si no, por ahora es ruido.
    // Algoritmo ingenuo: se compara cada punto contra TODOS los demás, O(n²) comparaciones.
    for (int i = 0; i < n; i++) {
        int neighbors = 0;
        for (int j = 0; j < n; j++)   // j = i también entra: el punto es su propio vecino (como sklearn)
            if (dist2(points[i], points[j]) <= EPS2)
                neighbors++;
        isCore[i] = (neighbors >= MIN_SAMPLES);
        labels[i] = isCore[i] ? CORE1 : NOISE;
    }

    // Paso 2: un punto que quedó como ruido pero está a <= EPS de un core de PRIMER orden se
    // re-etiqueta como core de segundo orden.
    // Decisión: se pregunta a isCore y no a labels. isCore sólo contiene los cores del paso 1, así un
    // punto que se acaba de volver CORE2 no puede "contagiar" a otros (CORE2 no hace alcanzables a
    // otros puntos). Esto es lo que hace que el resultado coincida con los puntos "border" de sklearn.
    for (int i = 0; i < n; i++) {
        if (isCore[i])
            continue;   // los cores de primer orden ya tienen su etiqueta final
        for (int j = 0; j < n; j++) {
            if (isCore[j] && dist2(points[i], points[j]) <= EPS2) {
                labels[i] = CORE2;
                break;   // basta con UN core cercano; no hace falta revisar el resto
            }
        }
    }

    double end = omp_get_wtime();
    // Se imprime SÓLO el tiempo en segundos: run_experiments.py lee esta línea como un número.
    cout << end - start << "\n";

    writeCSV(args[1].c_str(), points, labels, n);

    delete[] isCore;
    delete[] labels;
    freePoints(points, n);
    return 0;
}
