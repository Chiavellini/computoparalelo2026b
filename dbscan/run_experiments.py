import os
import subprocess
import sys

SIZES = [20000, 40000, 80000, 120000, 140000, 160000, 180000, 200000]
K_VALUES = [2, 4, 8, 16, 32, 64]
REPS = 10

cores = os.cpu_count()
THREADS = [1, cores // 2, cores, cores * 2]

folder = os.path.dirname(os.path.abspath(__file__))
output_name = "experiments.csv"
if "--quick" in sys.argv:
    SIZES = [4000, 20000]
    REPS = 2
    output_name = "experiments_quick.csv"

def run(program, n, args):
    """Runs one program on the n points file and returns the seconds it printed."""
    data = os.path.join(folder, "data", f"{n}_data.csv")
    result = os.path.join(folder, "data", f"{n}_results.csv")
    command = [os.path.join(folder, program), data, result] + [str(a) for a in args]
    output = subprocess.run(command, capture_output=True, text=True, check=True).stdout
    return float(output)

subprocess.run(["make", "-B"], cwd=folder, check=True, stdout=subprocess.DEVNULL)

missing = [n for n in SIZES if not os.path.exists(os.path.join(folder, "data", f"{n}_data.csv"))]
if missing:
    sys.exit(f"missing input files for n = {missing}; run: python3 generate_data.py "
             + " ".join(str(n) for n in missing))

configs = []
for n in SIZES:
    configs.append(("serial", n, 1, 0))
    for t in THREADS:
        configs.append(("p1", n, t, 0))
        for k in K_VALUES:
            configs.append(("p2", n, t, k))

os.makedirs(os.path.join(folder, "results"), exist_ok=True)
path = os.path.join(folder, "results", output_name)
with open(path, "w") as f:
    f.write("version,n,threads,k,rep,seconds\n")
    for number, (version, n, t, k) in enumerate(configs, start=1):
        for rep in range(1, REPS + 1):
            if version == "serial":
                seconds = run("dbscan_serial", n, [])
            elif version == "p1":
                seconds = run("dbscan_p1", n, [t])
            else:
                seconds = run("dbscan_p2", n, [t, k])
            f.write(f"{version},{n},{t},{k},{rep},{seconds}\n")
            f.flush()
        print(f"[{number}/{len(configs)}] {version} n={n} threads={t} k={k} last={seconds:.4f}s")

print("saved", path)
