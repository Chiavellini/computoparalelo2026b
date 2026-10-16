// Code shared by the three DBSCAN versions (serial, P1 and P2).
#ifndef COMMON_HPP
#define COMMON_HPP

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

// DBSCAN parameters (same values as the notebook DBSCAN_noise.ipynb)
const double EPS = 0.03;          // neighborhood radius
const double EPS2 = EPS * EPS;    // radius squared: compare squared distances, no sqrt needed
const int MIN_SAMPLES = 10;       // min points within EPS (the point itself included) to be dense

// labels written in the output csv
const int NOISE = 0;   // noise or outlier
const int CORE1 = 1;   // core point of first order
const int CORE2 = 2;   // core point of second order (noise reachable from a first order core)

///////////////////////////////////////////////////////////

// squared euclidean distance between two points p = (x, y) and q = (x, y)
inline double dist2(double* p, double* q) {
    double dx = p[0] - q[0];
    double dy = p[1] - q[1];
    return dx * dx + dy * dy;
}

///////////////////////////////////////////////////////////

// reads a csv with one point "x,y" per line into a n x 2 matrix; n is returned by reference
double** readCSV(const char* filename, int& n) {
    ifstream file(filename);
    if (!file) {
        cout << "Could not open " << filename << "\n";
        exit(1);
    }

    // first pass: count the points (non empty lines)
    string line;
    n = 0;
    while (getline(file, line))
        if (!line.empty())
            n++;

    // allocate memory for the matrix of points
    double** points = new double*[n];
    for (int i = 0; i < n; i++)
        points[i] = new double[2];

    // second pass: read the coordinates
    file.clear();
    file.seekg(0);
    char comma;
    for (int i = 0; i < n; i++)
        file >> points[i][0] >> comma >> points[i][1];

    return points;
}

///////////////////////////////////////////////////////////

// writes one line "x,y,label" per point
void writeCSV(const char* filename, double** points, int* labels, int n) {
    ofstream file(filename);
    file.setf(ios::fixed);
    file.precision(3);
    for (int i = 0; i < n; i++)
        file << points[i][0] << "," << points[i][1] << "," << labels[i] << "\n";
}

///////////////////////////////////////////////////////////

// releases the memory of the matrix of points
void freePoints(double** points, int n) {
    for (int i = 0; i < n; i++)
        delete[] points[i];
    delete[] points;
}

#endif
