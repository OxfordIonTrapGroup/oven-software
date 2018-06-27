
import numpy as np
import matplotlib.pyplot as plt


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



for i in range(2):
    try:
        data = np.loadtxt("current_vs_duty_{}.txt".format(i))
    except Exception as e:
        print("Skipping channel {}".format(i))
        continue

    duties = data[:,0]
    currents = data[:,1]
    temperatures = data[:,2]
    voltages = data[:,3]


    resistances = voltages/currents

    resistance_index_min = round(len(resistances)/2)
    resistance_index_max = len(resistances)

    mean_resistance = np.mean(resistances[resistance_index_min:resistance_index_max])


    plt.clf()
    fig, ax = plt.subplots(nrows=3, sharex=True)

    ax[0].plot(duties, currents, '.')
    ax[0].plot(duties[resistance_index_min:resistance_index_max],
        currents[resistance_index_min:resistance_index_max], 'r.')
    ax[0].set_ylabel("Current (A)")


    ax[1].plot(duties, voltages, '.')
    ax[1].plot(duties[resistance_index_min:resistance_index_max],
        voltages[resistance_index_min:resistance_index_max], 'r.')
    ax[1].set_ylabel("Voltage (V)")

    ax[2].plot(duties, temperatures, '.')
    ax[2].set_ylabel("Temperature (C)")

    ax[2].set_xlabel("Duty cycle (/1)")


    set_grid(ax[0])
    set_grid(ax[1])
    set_grid(ax[2])



    ax[1].set_title("Mean resistance = {:0.3f} Ohms".format(mean_resistance))

    plt.tight_layout()
    plt.savefig("current_vs_duty_{}.pdf".format(i))
