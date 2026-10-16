# Averages the repetitions, computes speedups and draws the charts.
#   speedup = mean serial time / mean time of the parallel version (same n)
# Usage: python3 plot_speedup.py                                 -> results/experiments.csv
#        python3 plot_speedup.py results/experiments_quick.csv   -> any other experiment file
# Writes into the folder of the input csv:
#   summary.csv           mean time and speedup of every configuration (table view)
#   speedup_p1.png        P1: speedup vs number of points, one line per thread count
#   speedup_p2.png        P2: one panel per thread count, one line per k
#   speedup_threads.png   largest n: speedup vs threads, P1 and P2 (each k)
import os
import sys

import matplotlib.pyplot as plt
import pandas as pd

folder = os.path.dirname(os.path.abspath(__file__))
path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(folder, "results", "experiments.csv")
out = os.path.dirname(os.path.abspath(path))

# fixed color order (colorblind-checked) and a different marker per series
COLORS = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#008300", "#4a3aa7"]
MARKERS = ["o", "s", "^", "D", "v", "P", "X"]
plt.rcParams.update({"font.size": 10, "axes.spines.top": False, "axes.spines.right": False,
                     "axes.edgecolor": "#888888", "axes.grid": True, "grid.color": "#e6e6e6",
                     "grid.linewidth": 0.8, "axes.axisbelow": True})

# mean of the repetitions of every configuration
df = pd.read_csv(path)
summary = df.groupby(["version", "n", "threads", "k"], as_index=False)["seconds"].mean()
serial = summary[summary.version == "serial"].set_index("n")["seconds"]
summary["speedup"] = summary["n"].map(serial) / summary["seconds"]
summary.to_csv(os.path.join(out, "summary.csv"), index=False)
print(summary.to_string(index=False))

p1 = summary[summary.version == "p1"]
p2 = summary[summary.version == "p2"]
threads = sorted(p1.threads.unique())
ks = sorted(p2.k.unique())


def reference_line(ax):
    """Dashed line at speedup 1.5, the minimum required by the project (NOTA 1)."""
    ax.axhline(1.5, color="#888888", linestyle="--", linewidth=1)
    ax.annotate("1.5x", xy=(1, 1.5), xycoords=("axes fraction", "data"),
                xytext=(2, 2), textcoords="offset points", color="#555555", fontsize=8)


def line(ax, x, y, i, label):
    ax.plot(x, y, color=COLORS[i], marker=MARKERS[i], linewidth=2, markersize=5, label=label)


# Chart 1: P1, speedup vs number of points
fig, ax = plt.subplots(figsize=(7, 4.2))
for i, t in enumerate(threads):
    rows = p1[p1.threads == t]
    line(ax, rows.n, rows.speedup, i, f"{t} threads")
reference_line(ax)
ax.set_title("P1: speedup vs serial")
ax.set_xlabel("Number of points")
ax.set_ylabel("Speedup")
ax.legend(frameon=False)
fig.tight_layout()
fig.savefig(os.path.join(out, "speedup_p1.png"), dpi=150)

# Chart 2: P2, one panel per thread count, one line per k (same y scale in every panel)
fig, axes = plt.subplots(1, len(threads), figsize=(3.2 * len(threads), 3.8), sharey=True)
for ax, t in zip(axes, threads):
    for i, k in enumerate(ks):
        rows = p2[(p2.threads == t) & (p2.k == k)]
        line(ax, rows.n, rows.speedup, i, f"k = {k}")
    reference_line(ax)
    ax.set_title(f"{t} threads")
    ax.set_xlabel("Number of points")
axes[0].set_ylabel("Speedup")
axes[-1].legend(frameon=False, loc="upper left", bbox_to_anchor=(1, 1))
fig.suptitle("P2: speedup vs serial")
fig.tight_layout()
fig.savefig(os.path.join(out, "speedup_p2.png"), dpi=150)

# Chart 3: largest n, speedup vs threads for P1 and every k of P2
n_max = summary.n.max()
fig, ax = plt.subplots(figsize=(7, 4.2))
rows = p1[p1.n == n_max]
line(ax, rows.threads, rows.speedup, 0, "P1")
for i, k in enumerate(ks, start=1):
    rows = p2[(p2.n == n_max) & (p2.k == k)]
    line(ax, rows.threads, rows.speedup, i, f"P2, k = {k}")
reference_line(ax)
ax.set_xscale("log", base=2)
ax.set_xticks(threads)
ax.set_xticklabels([str(t) for t in threads])
ax.set_title(f"Speedup vs threads ({n_max} points)")
ax.set_xlabel("Threads")
ax.set_ylabel("Speedup")
ax.legend(frameon=False, loc="upper left", bbox_to_anchor=(1, 1))
fig.tight_layout()
fig.savefig(os.path.join(out, "speedup_threads.png"), dpi=150)

print("charts saved in", out)
