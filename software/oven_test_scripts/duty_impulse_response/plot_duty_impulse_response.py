
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



for i in range(2):
    try:
        f = open("duty_impulse_response_data_{}.pkl".format(i), "rb")
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


    I_max = np.max(results["I"])
    # Find the last sample which had the maximum current
    last_max_index = -np.argmax(results["I"][::-1] > 0.95*I_max) - 1

    # Find the next sample which has a 'near zero' current
    next_min_index = np.argmax(results["I"][last_max_index::] > 0.1)

    dt = t[next_min_index] - t[last_max_index]
    dT = results["T"][next_min_index] - results["T"][last_max_index]
    #dI = results["I"][next_min_index] - results["I"][last_max_index]
    dI = I_max

    ax[0].plot([t[last_max_index], t[last_max_index]],
        [np.min(results["T"]), np.max(results["T"])], '-')


    set_grid(ax[0])
    set_grid(ax[1])
    set_grid(ax[2])



    dTdI = dT/dI

    output = "Temperature change: {:f} C ({:f} - {:f})\n".format(dT, results["T"][next_min_index], results["T"][last_max_index])
    output += "Current change: {:f} A\n".format(dI)
    output += "T-C coefficent: {:f} C/A".format(dTdI)

    print(output)

    ax[0].set_title(output)

    plt.tight_layout()
    plt.savefig("duty_impulse_response_data_{}.pdf".format(i))
