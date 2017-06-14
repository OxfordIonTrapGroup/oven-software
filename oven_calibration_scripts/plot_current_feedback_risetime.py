
import numpy as np
import matplotlib.pyplot as plt
import pickle

f = open("measure_current_feedback_risetime_data.pkl", "rb")
results = pickle.load(f)

fig, ax = plt.subplots(nrows=3, sharex=True)
t = np.arange(len(results["T"]))/1000

ax[0].plot(t, results["T"], '.')
ax[1].plot(t, results["I"], '.')
ax[2].plot(t, results["V"], '.')
ax[2].plot(t, results["V_out"], '.')


I_max = np.max(results["I"])
# Find the last sample which had the maximum current
last_max_index = np.argmax(results["I"][::-1])

# Find the next sample which has a 'near zero' current
next_min_index = np.argmax(results["I"][last_max_index::] > 0.1)

dt = t[next_min_index] - t[last_max_index]
dT = results["T"][next_min_index] - results["T"][last_max_index]
#dI = results["I"][next_min_index] - results["I"][last_max_index]
dI = - results["I"][last_max_index]

dTdI = dT/dI

print("Fall time: {:f} s".format(dt))
print("Temperature change: {:f} C".format(dT))
print("Current change: {:f} A".format(dI))
print("T-C coefficent: {:f} C/A".format(dTdI))



plt.savefig("measure_current_feedback_risetime_data.pdf")
