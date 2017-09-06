
import numpy as np
import time
import atomic_oven_controller

channel = int(input("Oven channel to test (0,1): "))
if channel < 0 or channel > 1:
    raise Exception("Bad channel: {}".format(channel))

p = atomic_oven_controller.OvenPICInterface()

min_duty = 0
max_duty = 0.02

n_steps = 101

t_per_step = 0.1

duties = np.linspace(min_duty, max_duty, n_steps)
currents = np.zeros(n_steps)
temperatures = np.zeros(n_steps)
voltages = np.zeros(n_steps)

for i in range(n_steps):
    p.set_pwm_duty(channel, duties[i])
    time.sleep(t_per_step)
    values = p.adc_read_calibrated_sample()
    currents[i] = values[channel]["current"]
    temperatures[i] = values[channel]["temperature"]
    voltages[i] = values[channel]["output_voltage"]

    print(
        duties[i],
        values[channel]["current"],
        values[channel]["temperature"],
        values[channel]["output_voltage"])

p.set_pwm_duty(channel, 0)


np.savetxt("current_vs_duty_{}.txt".format(channel), np.c_[duties, currents, temperatures, voltages])
