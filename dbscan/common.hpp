#ifndef COMMON_HPP
#define COMMON_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

const double EPS = 0.03;
const double EPS2 = EPS * EPS;
const int MIN_SAMPLES = 10;

const int NOISE = 0;
const int CORE1 = 1;
const int CORE2 = 2;

const string DEFAULT_INPUT = "data/10000_data.csv";
const string DEFAULT_OUTPUT = "data/10000_results.csv";
const int DEFAULT_K = 4;

vector<string> getArgs(int argc, char* argv[], const vector<string>& defaults, const string& usage) {
    int expected = (int)defaults.size();

    if (argc == 1) {
        cerr << "Sin argumentos, se usan los valores por defecto:";
        for (int i = 0; i < expected; i++)
            cerr << " " << defaults[i];
        cerr << "\n";
        return defaults;
    }

    if (argc - 1 < expected) {
        cout << "Uso: " << argv[0] << " " << usage << "\n";
        exit(1);
    }

    return vector<string>(argv + 1, argv + 1 + expected);
}

inline double dist2(double* p, double* q) {
    double dx = p[0] - q[0];
    double dy = p[1] - q[1];
    return dx * dx + dy * dy;
}

double** readCSV(const char* filename, int& n) {
    ifstream file(filename);
    if (!file) {
        cout << "No se pudo abrir " << filename << "\n"
             << "Genera los datos primero desde dbscan/: python3 generate_data.py N "
             << "(o corre la primera celda de DBSCAN_noise.ipynb)\n";
        exit(1);
    }

    string line;
    n = 0;
    while (getline(file, line))
        if (!line.empty())
            n++;

    double** points = new double*[n];
    for (int i = 0; i < n; i++)
        points[i] = new double[2];

    file.clear();
    file.seekg(0);
    char comma;
    for (int i = 0; i < n; i++)
        file >> points[i][0] >> comma >> points[i][1];

    return points;
}

void writeCSV(const char* filename, double** points, int* labels, int n) {
    ofstream file(filename);
    if (!file) {

        cout << "No se pudo escribir " << filename << " (¿existe la carpeta?)\n";
        exit(1);
    }
    file.setf(ios::fixed);
    file.precision(3);
    for (int i = 0; i < n; i++)
        file << points[i][0] << "," << points[i][1] << "," << labels[i] << "\n";
}

void freePoints(double** points, int n) {
    for (int i = 0; i < n; i++)
        delete[] points[i];
    delete[] points;
}

#endif
