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
    bool* isCore = new bool[n];
    int* labels = new int[n];

    double start = omp_get_wtime();

    int rows = 1;
    for (int r = 1; r * r <= k; r++)
        if (k % r == 0)
            rows = r;
    int cols = k / rows;

    double minX = points[0][0], maxX = points[0][0];
    double minY = points[0][1], maxY = points[0][1];
    for (int i = 1; i < n; i++) {
        if (points[i][0] < minX) minX = points[i][0];
        if (points[i][0] > maxX) maxX = points[i][0];
        if (points[i][1] < minY) minY = points[i][1];
        if (points[i][1] > maxY) maxY = points[i][1];
    }
    double width = (maxX - minX) / cols;
    double height = (maxY - minY) / rows;

    vector<vector<int>> owned(k);
    for (int i = 0; i < n; i++) {
        int c = (int)((points[i][0] - minX) / width);
        int r = (int)((points[i][1] - minY) / height);
        if (c == cols) c = cols - 1;
        if (r == rows) r = rows - 1;
        owned[r * cols + c].push_back(i);
    }

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
    cout << end - start << "\n";

    writeCSV(args[1].c_str(), points, labels, n);

    delete[] isCore;
    delete[] labels;
    freePoints(points, n);
    return 0;
}
