// Código compartido por las tres versiones de DBSCAN (serial, P1 y P2).
//
// Decisión: todo lo que NO es el algoritmo (parámetros, distancia, lectura/escritura de csv y
// argumentos) vive aquí, en un solo lugar. Así las tres versiones usan exactamente lo mismo y:
//   - las diferencias de tiempo entre versiones se deben sólo a la estrategia de paralelización;
//   - los resultados de las tres se pueden comparar línea por línea (deben ser idénticos).
#ifndef COMMON_HPP
#define COMMON_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

// Parámetros de DBSCAN. Son los mismos que usa la libreta DBSCAN_noise.ipynb con sklearn,
// DBSCAN(eps=0.03, min_samples=10), para poder validar nuestros resultados contra sklearn.
const double EPS = 0.03;          // radio de la vecindad (épsilon)
const double EPS2 = EPS * EPS;    // radio al cuadrado: comparamos distancias al cuadrado (ver dist2)
const int MIN_SAMPLES = 10;       // mínimo de puntos a distancia <= EPS para que la zona sea densa.
                                  // El punto se cuenta a sí mismo como vecino, igual que sklearn.

// Etiquetas que se escriben en la tercera columna del csv de salida.
// La libreta usa esta columna directamente como color de cada punto.
const int NOISE = 0;   // ruido u outlier
const int CORE1 = 1;   // punto core de primer orden: está en medio de una zona densa
const int CORE2 = 2;   // punto core de segundo orden: no es denso, pero está a <= EPS de un CORE1

// Valores que se usan cuando el programa se ejecuta SIN argumentos, por ejemplo con el botón ▶
// de VS Code (Code Runner compila y ejecuta el programa sin pasarle argumentos).
// Las rutas son relativas a la carpeta dbscan/: ahí ejecuta Code Runner el programa y ahí guarda
// la libreta los csv, así que el resultado queda justo donde la libreta lo va a buscar.
const string DEFAULT_INPUT = "data/4000_data.csv";
const string DEFAULT_OUTPUT = "data/4000_results.csv";
const int DEFAULT_K = 16;

///////////////////////////////////////////////////////////

// Regresa los argumentos de la línea de comandos (sin el nombre del programa).
//   - Si vienen todos, los usa tal cual (así los llama run_experiments.py).
//   - Si no viene ninguno, usa |defaults| y avisa cuáles se usaron.
//   - Si vienen algunos pero no todos, muestra cómo se usa el programa y termina.
// El aviso va a cerr y no a cout porque cout debe tener SÓLO el tiempo: los scripts lo leen.
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

///////////////////////////////////////////////////////////

// Distancia euclidiana AL CUADRADO entre dos puntos p = (x, y) y q = (x, y).
// Decisión: como ambos lados son positivos, d <= EPS es lo mismo que d² <= EPS². Comparamos contra
// EPS2 y nos ahorramos la raíz cuadrada, que es la operación más cara y se haría unas n² veces.
// inline: se llama en el ciclo más interno, así le pedimos al compilador que no haga una llamada.
inline double dist2(double* p, double* q) {
    double dx = p[0] - q[0];
    double dy = p[1] - q[1];
    return dx * dx + dy * dy;
}

///////////////////////////////////////////////////////////

// Lee un csv con un punto "x,y" por línea y regresa una matriz de n x 2; n se regresa por referencia.
// Decisión: la matriz es double** (un arreglo de filas), el mismo estilo de las matrices del curso
// (int** graph en emptyfloydwarshall.cpp). Esta lectura no se mide en los tiempos.
double** readCSV(const char* filename, int& n) {
    ifstream file(filename);
    if (!file) {
        cout << "No se pudo abrir " << filename << "\n"
             << "Genera los datos primero desde dbscan/: python3 generate_data.py N "
             << "(o corre la primera celda de DBSCAN_noise.ipynb)\n";
        exit(1);
    }

    // primera pasada: contar los puntos (líneas no vacías) para reservar exactamente n filas
    string line;
    n = 0;
    while (getline(file, line))
        if (!line.empty())
            n++;

    // reservar la memoria de la matriz de puntos
    double** points = new double*[n];
    for (int i = 0; i < n; i++)
        points[i] = new double[2];

    // segunda pasada: regresar al inicio del archivo y leer las coordenadas;
    // |comma| sólo sirve para saltarse la coma entre x y y
    file.clear();
    file.seekg(0);
    char comma;
    for (int i = 0; i < n; i++)
        file >> points[i][0] >> comma >> points[i][1];

    return points;
}

///////////////////////////////////////////////////////////

// Escribe una línea "x,y,etiqueta" por punto, en el MISMO orden que la entrada.
// Decisiones: mantener el orden permite comparar contra sklearn punto por punto (check.py), y
// 3 decimales es la misma precisión con la que la libreta genera los datos.
void writeCSV(const char* filename, double** points, int* labels, int n) {
    ofstream file(filename);
    if (!file) {
        // pasa, por ejemplo, si la carpeta de salida no existe; sin este aviso no se escribiría nada
        cout << "No se pudo escribir " << filename << " (¿existe la carpeta?)\n";
        exit(1);
    }
    file.setf(ios::fixed);
    file.precision(3);
    for (int i = 0; i < n; i++)
        file << points[i][0] << "," << points[i][1] << "," << labels[i] << "\n";
}

///////////////////////////////////////////////////////////

// Libera la memoria de la matriz de puntos (cada fila y después el arreglo de filas).
void freePoints(double** points, int n) {
    for (int i = 0; i < n; i++)
        delete[] points[i];
    delete[] points;
}

#endif
