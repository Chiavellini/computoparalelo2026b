# Checks a result csv of our programs against sklearn's DBSCAN, label by label:
#   0 noise  <-> sklearn noise (label -1)
#   1 core   <-> sklearn core samples (core_sample_indices_)
#   2 core2  <-> sklearn border points (in a cluster but not core)
# Usage: python3 check.py data/20000_data.csv data/20000_results.csv
import sys

import numpy as np
from sklearn.cluster import DBSCAN

data = np.loadtxt(sys.argv[1], delimiter=",")
result = np.loadtxt(sys.argv[2], delimiter=",")

# the output must keep the points in the same order as the input
assert len(data) == len(result) and np.allclose(data, result[:, :2]), "points differ"

db = DBSCAN(eps=0.03, min_samples=10).fit(data)
expected = np.zeros(len(data), dtype=int)     # 0 noise
expected[db.labels_ != -1] = 2                # 2 border = core of second order
expected[db.core_sample_indices_] = 1         # 1 core of first order
ours = result[:, 2].astype(int)

mismatches = int(np.sum(expected != ours))
counts = lambda labels: np.bincount(labels, minlength=3).tolist()
print(f"points: {len(data)}  [noise, core1, core2] sklearn: {counts(expected)}  "
      f"ours: {counts(ours)}  mismatches: {mismatches}")
sys.exit(1 if mismatches else 0)
