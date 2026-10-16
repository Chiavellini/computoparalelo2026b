# Generates the input csv files (same generator as the notebook DBSCAN_noise.ipynb).
# Usage: python3 generate_data.py            -> all the experiment sizes
#        python3 generate_data.py 4000 20000 -> only the given sizes
import os
import sys

import numpy as np
from sklearn.datasets import make_blobs

SIZES = [20000, 40000, 80000, 120000, 140000, 160000, 180000, 200000]

folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
os.makedirs(folder, exist_ok=True)

sizes = [int(s) for s in sys.argv[1:]] or SIZES
for n_points in sizes:
    # randomly generating data points and noise
    points, y_true = make_blobs(n_samples=n_points,
                                centers=4,
                                cluster_std=0.06,
                                random_state=11,
                                center_box=(0, 1.0))

    # only positive points and with three decimals
    points = np.round(np.abs(points[:, ::-1]), 3)

    path = os.path.join(folder, f"{n_points}_data.csv")
    np.savetxt(path, points, delimiter=",", fmt="%.3f")
    print("wrote", path)
