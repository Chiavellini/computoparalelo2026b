# Checks a result csv of our programs against sklearn's DBSCAN.
# Usage: python3 check.py data/20000_data.csv data/20000_results.csv
import sys

import numpy as np
from sklearn.cluster import DBSCAN

data = np.loadtxt(sys.argv[1], delimiter=",")
result = np.loadtxt(sys.argv[2], delimiter=",")

# the output must keep the points in the same order as the input
assert len(data) == len(result) and np.allclose(data, result[:, :2]), "points differ"

sklearn_noise = DBSCAN(eps=0.03, min_samples=10).fit_predict(data) == -1
our_noise = result[:, 2] == 0

mismatches = int(np.sum(sklearn_noise != our_noise))
print(f"points: {len(data)}  noise sklearn: {sklearn_noise.sum()}  "
      f"noise ours: {our_noise.sum()}  mismatches: {mismatches}")
sys.exit(1 if mismatches else 0)
