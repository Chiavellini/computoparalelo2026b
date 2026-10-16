// Naive DBSCAN for noise detection - serial version.
// Usage: ./dbscan_serial input.csv output.csv
#include <omp.h>
#include "common.hpp"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " input.csv output.csv\n";
        return 1;
    }

    int n = 0;
    double** points = readCSV(argv[1], n);
    bool* isCore = new bool[n];   // result of step 1 (first order cores)
    int* labels = new int[n];     // final label of every point

    double start = omp_get_wtime();

    // Step 1: a point is a first order core if it has at least MIN_SAMPLES
    // points (itself included) at distance <= EPS; otherwise it is noise.
    for (int i = 0; i < n; i++) {
        int neighbors = 0;
        for (int j = 0; j < n; j++)
            if (dist2(points[i], points[j]) <= EPS2)
                neighbors++;
        isCore[i] = (neighbors >= MIN_SAMPLES);
        labels[i] = isCore[i] ? CORE1 : NOISE;
    }

    // Step 2: a noise point that is at distance <= EPS of a first order core
    // is relabeled as a second order core.
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
    cout << end - start << "\n";   // only the time, so scripts can read it

    writeCSV(argv[2], points, labels, n);

    delete[] isCore;
    delete[] labels;
    freePoints(points, n);
    return 0;
}
