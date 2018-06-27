import time
import pickle
import argparse

import atomic_oven_controller.interface

parser = argparse.ArgumentParser()
parser.add_argument("channel", type=int)
args = parser.parse_args()


if args.channel < 0 or args.channel > 1:
    raise Exception("Bad channel: {}".format(args.channel))


p = atomic_oven_controller.interface.Interface()

on_duty = 0.05
#on_duty = 0.01
dt = 0.05

p.set_pwm_duty(args.channel, 0)

p.adc_decimate(0)

p.adc_start_streaming(range(0,8))

time.sleep(dt)
p.set_pwm_duty(args.channel, on_duty)

time.sleep(dt)
p.set_pwm_duty(args.channel, 0)
time.sleep(dt)


results = p.adc_stop_streaming()[args.channel]

f = open("duty_impulse_response_data_{}.pkl".format(args.channel), "wb")
pickle.dump(results, f, -1)
