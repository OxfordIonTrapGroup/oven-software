
import numpy as np
import matplotlib.pyplot as plt
import pickle

f = open("measure_temperature_feedback_data.pkl", "rb")
results = pickle.load(f)

fig, ax = plt.subplots(nrows=3, sharex=True)
t = (np.arange(len(results["T"]))/100)-1

ax[0].plot(t, results["T"], '.')
ax[1].plot(t, results["I"], '.')

#ax[2].plot(t, results["V"]/results["I"], '.')
#ax[2].set_ylim(0.14,0.2)

ax[2].plot(t, results["V"], '.')
ax[2].plot(t, results["V_out"], '.')

#ax[2].set_xticks(np.arange(0,10))

major_ticks = np.arange(-2, np.max(t), 1)                                              
minor_ticks = np.arange(-2, np.max(t), 0.5) 


for i in range(3):
    ax[i].set_xticks(major_ticks)
    ax[i].set_xticks(minor_ticks, minor=True)
    ax[i].grid(c='grey', linestyle='--', which='major', alpha=0.5)
    ax[i].grid(c='grey', linestyle='--', which='minor', alpha=0.2)
    ax[i].set_axisbelow(True)

plt.tight_layout()
plt.show()
plt.savefig("measure_temperature_feedback_data.pdf")
