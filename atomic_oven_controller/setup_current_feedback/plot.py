
import numpy as np
import matplotlib.pyplot as plt
import pickle


def set_grid(ax):
    major_ticks = ax.get_xticks(minor=False)
    minor_ticks = np.arange(major_ticks[0],
        major_ticks[-1], (major_ticks[1] - major_ticks[0])/2)
    ax.set_xticks(minor_ticks, minor=True)

    major_ticks = ax.get_yticks(minor=False)
    minor_ticks = np.arange(major_ticks[0],
        major_ticks[-1], (major_ticks[1] - major_ticks[0])/2)
    ax.set_yticks(minor_ticks, minor=True)

    ax.grid(c='grey', linestyle='--', which='major', alpha=0.5)
    ax.grid(c='grey', linestyle='--', which='minor', alpha=0.2)
    ax.set_axisbelow(True)


for channel in range(2):
    try:
        f = open("current_feedback_risetime_data_{}.pkl".format(channel), "rb")
    except Exception as e:
        print("Skipping channel {}".format(i))
        continue

    results = pickle.load(f)

    fig, ax = plt.subplots(nrows=3, sharex=True)
    t = np.arange(len(results["T"]))

    ax[0].plot(t, results["T"], '.')
    ax[0].set_ylabel("Temperature (C)")


    ax[1].plot(t, results["I"], '.')
    ax[1].set_ylabel("Current (A)")

    ax[2].plot(t, results["V_out"], '.')
    ax[2].set_ylabel("Voltage (V)")

    ax[2].set_xlabel("Time (ms)")


    set_grid(ax[0])
    set_grid(ax[1])
    set_grid(ax[2])


    plt.savefig("current_feedback_risetime_data_{}.pdf".format(channel))
