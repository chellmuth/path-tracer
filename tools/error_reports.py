from pathlib import Path

import matplotlib
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import pyexr

def calculate_mse(test, gt):
    return np.sum((gt - test) ** 2)

def build_dataset(gt, data_directory):
    mses = []

    i = 0
    while True:
        exr_path = Path(f"../{data_directory}/auto-{2**i:05d}spp.exr")
        if not exr_path.exists():
            break

        mse = calculate_mse(
            pyexr.read(str(exr_path)),
            gt
        )
        mses.append(mse)

        i += 1

    return mses

def build_datasets(gt, plots):
    return [
        build_dataset(gt, data_directory)
        for _, data_directory
        in plots
    ]

@ticker.FuncFormatter
def power_of_two_formatter(x, pos):
    return int(2 ** x)

def run(gt, plots):
    ys_list = build_datasets(gt, plots)

    fig, ax = plt.subplots()
    for i, ys in enumerate(ys_list):
        xs = list(range(len(ys)))
        ax.plot(xs, ys, label=plots[i][0])

    ax.set(xlabel='spp', ylabel='mse', title='Convergence Plot')
    ax.xaxis.set_major_formatter(power_of_two_formatter)

    plt.legend(loc='upper right')

    plt.show()

if __name__ == "__main__":
    gt = pyexr.read("../path-traced/auto-08192spp.exr")

    run(gt, [("Path Traced", "path-traced"), ("Ours", "pdf-test")])
