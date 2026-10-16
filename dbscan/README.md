# DBSCAN paralelo para la detección de ruido (OpenMP)

| File | What it is |
|---|---|
| `common.hpp` | Parameters (`EPS=0.03`, `MIN_SAMPLES=10`), labels, distance, csv read/write |
| `dbscan_serial.cpp` | Serial naive DBSCAN (step 1: first order cores, step 2: second order cores) |
| `dbscan_p1.cpp` | P1: the whole matrix as one block, `parallel for` over the points |
| `dbscan_p2.cpp` | P2: space split into `k` blocks (+ halo of width EPS), `parallel for` over the blocks |
| `generate_data.py` | Input csv files (same generator as `DBSCAN_noise.ipynb`) |
| `check.py` | Compares a result csv against sklearn's DBSCAN |
| `run_experiments.py` | Runs the whole experiment, 10 repetitions → `results/experiments.csv` |
| `plot_speedup.py` | Averages, speedups (`results/summary.csv`) and charts (`results/*.png`) |

Output csv: `x,y,label` with label `0` = noise, `1` = first order core, `2` = second order core.
It can be plotted with the second cell of `DBSCAN_noise.ipynb`.

## Build

```bash
make
```

The Makefile uses `g++ -fopenmp` on Windows/Linux and `clang++` + Homebrew `libomp` on macOS.
Without make (class setup):

```bash
g++ -O2 -fopenmp dbscan_serial.cpp -o dbscan_serial
g++ -O2 -fopenmp dbscan_p1.cpp -o dbscan_p1
g++ -O2 -fopenmp dbscan_p2.cpp -o dbscan_p2
```

## Run one version

```bash
python3 generate_data.py 20000
./dbscan_serial data/20000_data.csv data/20000_results.csv
./dbscan_p1     data/20000_data.csv data/20000_results.csv 8
./dbscan_p2     data/20000_data.csv data/20000_results.csv 8 16
python3 check.py data/20000_data.csv data/20000_results.csv
```

Each program prints only the algorithm time in seconds (csv reading/writing is not timed).

## Full experiment

```bash
python3 generate_data.py
python3 run_experiments.py
python3 plot_speedup.py
```

Sizes {20000 … 200000}, threads {1, cores/2, cores, cores·2}, k {2, 4, 8, 16, 32, 64}, 10 repetitions.
On the M2 (8 cores) it takes about 3 hours. Close other apps and keep the Mac plugged in and awake
(e.g. `caffeinate -i python3 run_experiments.py`), since background load changes the times.
`python3 run_experiments.py --quick` is a 10-second smoke test that writes `results/experiments_quick.csv`.
