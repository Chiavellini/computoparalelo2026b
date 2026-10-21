import sys

import numpy as np
from sklearn.cluster import DBSCAN

data = np.loadtxt(sys.argv[1], delimiter=",")
result = np.loadtxt(sys.argv[2], delimiter=",")

assert len(data) == len(result) and np.allclose(data, result[:, :2]), "points differ"

db = DBSCAN(eps=0.03, min_samples=10).fit(data)
expected = np.zeros(len(data), dtype=int)
expected[db.labels_ != -1] = 2
expected[db.core_sample_indices_] = 1
ours = result[:, 2].astype(int)

mismatches = int(np.sum(expected != ours))
counts = lambda labels: np.bincount(labels, minlength=3).tolist()
print(f"points: {len(data)}  [noise, core1, core2] sklearn: {counts(expected)}  "
      f"ours: {counts(ours)}  mismatches: {mismatches}")
sys.exit(1 if mismatches else 0)
