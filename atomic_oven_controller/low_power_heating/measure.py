import numpy as np
import time
import argparse

import atomic_oven_controller.interface


parser = argparse.ArgumentParser()
parser.add_argument("channel", type=int)
args = parser.parse_args()

if args.channel < 0 or args.channel > 1:
    raise Exception("Bad channel: {}".format(args.channel))

p = atomic_oven_controller.interface.Interface()

min_duty = 0
max_duty = 0.025

n_steps = 101

t_per_step = 0.1

duties = np.linspace(min_duty, max_duty, n_steps)
currents = np.zeros(n_steps)
temperatures = np.zeros(n_steps)
voltages = np.zeros(n_steps)

for i in range(n_steps):
    p.set_pwm_duty(args.channel, duties[i])
    time.sleep(t_per_step)
    values = p.adc_read_calibrated_sample()
    currents[i] = values[args.channel]["current"]
    temperatures[i] = values[args.channel]["temperature"]
    voltages[i] = values[args.channel]["output_voltage"]

    print(
        duties[i],
        values[args.channel]["current"],
        values[args.channel]["temperature"],
        values[args.channel]["output_voltage"])

p.set_pwm_duty(args.channel, 0)


np.savetxt("current_vs_duty_{}.txt".format(args.channel), np.c_[duties, currents, temperatures, voltages])
