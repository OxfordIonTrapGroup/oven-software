
import numpy as np
import matplotlib.pyplot as plt
import pickle

f = open("measure_current_feedback_data.pkl", "rb")
results = pickle.load(f)

fig, ax = plt.subplots(nrows=3, sharex=True)
t = np.arange(len(results["T"]))

ax[0].plot(t, results["T"], '.')
ax[1].plot(t, results["I"], '.')
ax[2].plot(t, results["V"], '.')
ax[2].plot(t, results["V_out"], '.')

plt.savefig("measure_current_feedback_data.pdf")
