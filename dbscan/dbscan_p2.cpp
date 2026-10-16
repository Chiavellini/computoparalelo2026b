// Naive DBSCAN for noise detection - parallel version P2.
// The space is divided into k blocks (a grid of rows x cols). Each block is a
// task that one thread processes; blocks are handed out to threads dynamically.
//
// Consistency between blocks: every point is OWNED by exactly one block (only
// that block writes its label), and each block also reads the points of its
// neighbors that lie within EPS of its border (the "halo"). So a point near a
// border still sees all of its neighbors and gets the same label as in the
// serial version.
// Usage: ./dbscan_p2 input.csv output.csv threads k
#include <omp.h>
#include <vector>
#include "common.hpp"

int main(int argc, char* argv[]) {
    if (argc < 5) {
        cout << "Usage: " << argv[0] << " input.csv output.csv threads k\n";
        return 1;
    }
    int threads = atoi(argv[3]);
    int k = atoi(argv[4]);
    omp_set_num_threads(threads);

    int n = 0;
    double** points = readCSV(argv[1], n);
    bool* isCore = new bool[n];   // result of step 1 (first order cores)
    int* labels = new int[n];     // final label of every point

    double start = omp_get_wtime();

    // Grid of rows x cols = k blocks, as square as possible:
    // k=2 -> 1x2, 4 -> 2x2, 8 -> 2x4, 16 -> 4x4, 32 -> 4x8, 64 -> 8x8
    int rows = 1;
    for (int r = 1; r * r <= k; r++)
        if (k % r == 0)
            rows = r;
    int cols = k / rows;

    // Bounding box of the data
    double minX = points[0][0], maxX = points[0][0];
    double minY = points[0][1], maxY = points[0][1];
    for (int i = 1; i < n; i++) {
        if (points[i][0] < minX) minX = points[i][0];
        if (points[i][0] > maxX) maxX = points[i][0];
        if (points[i][1] < minY) minY = points[i][1];
        if (points[i][1] > maxY) maxY = points[i][1];
    }
    double width = (maxX - minX) / cols;    // width of one block
    double height = (maxY - minY) / rows;   // height of one block

    // owned[b]: points that lie inside block b (block b writes their labels)
    vector<vector<int>> owned(k);
    for (int i = 0; i < n; i++) {
        int c = (int)((points[i][0] - minX) / width);
        int r = (int)((points[i][1] - minY) / height);
        if (c == cols) c = cols - 1;   // points on the right/top edge of the box
        if (r == rows) r = rows - 1;
        owned[r * cols + c].push_back(i);
    }

    // candidates[b]: points that can be neighbors of the points of block b,
    // i.e. the points inside block b grown by EPS on every side (block + halo).
    // The extra 1e-9 protects against rounding errors at the borders.
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
                candidates[b].push_back(i);   // each thread only fills its own block b
    }

    // Step 1: every block labels its own points, comparing them only against
    // its candidates. Block b only writes isCore/labels of points it owns.
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
    // implicit barrier: step 2 starts only when isCore is complete in ALL
    // blocks, so the halo points of every block already have their final value

    // Step 2: noise points of block b look for a first order core among the
    // candidates of block b.
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
                    break;
                }
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
