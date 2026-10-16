// Naive DBSCAN for noise detection - parallel version P1.
// The whole matrix of points is treated as one indivisible block: the
// iterations of the outer loop (one per point) are split among the threads.
// Usage: ./dbscan_p1 input.csv output.csv threads
#include <omp.h>
#include "common.hpp"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cout << "Usage: " << argv[0] << " input.csv output.csv threads\n";
        return 1;
    }
    int threads = atoi(argv[3]);
    omp_set_num_threads(threads);

    int n = 0;
    double** points = readCSV(argv[1], n);
    bool* isCore = new bool[n];   // result of step 1 (first order cores)
    int* labels = new int[n];     // final label of every point

    double start = omp_get_wtime();

    // Step 1: each thread takes chunks of 64 points. Iteration i only writes
    // isCore[i] and labels[i], so there are no race conditions.
    #pragma omp parallel for schedule(dynamic, 64)
    for (int i = 0; i < n; i++) {
        int neighbors = 0;   // declared inside the loop, so it is private
        for (int j = 0; j < n; j++)
            if (dist2(points[i], points[j]) <= EPS2)
                neighbors++;
        isCore[i] = (neighbors >= MIN_SAMPLES);
        labels[i] = isCore[i] ? CORE1 : NOISE;
    }
    // implicit barrier: step 2 starts only when isCore is complete

    // Step 2: isCore is only read, labels[i] is only written by iteration i.
    #pragma omp parallel for schedule(dynamic, 64)
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
