#include <omp.h>
#include "common.hpp"

int main(int argc, char* argv[]) {
    vector<string> args = getArgs(argc, argv, {DEFAULT_INPUT, DEFAULT_OUTPUT},
                                  "input.csv output.csv");

    int n = 0;
    double** points = readCSV(args[0].c_str(), n);
    bool* isCore = new bool[n];
    int* labels = new int[n];

    double start = omp_get_wtime();

    for (int i = 0; i < n; i++) {
        int neighbors = 0;
        for (int j = 0; j < n; j++)
            if (dist2(points[i], points[j]) <= EPS2)
                neighbors++;
        isCore[i] = (neighbors >= MIN_SAMPLES);
        labels[i] = isCore[i] ? CORE1 : NOISE;
    }

    for (int i = 0; i < n; i++) {
        if (isCore[i])
            continue;
        for (int j = 0; j < n; j++) {
            if (isCore[j] && dist2(points[i], points[j]) <= EPS2) {
                labels[i] = CORE2;
                break;
            }
        }
    }

    double end = omp_get_wtime();

    cout << end - start << "\n";

    writeCSV(args[1].c_str(), points, labels, n);

    delete[] isCore;
    delete[] labels;
    freePoints(points, n);
    return 0;
}
