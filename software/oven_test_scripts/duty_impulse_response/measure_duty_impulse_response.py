

import time
import pickle

import atomic_oven_controller

channel = int(input("Oven channel to test (0,1): "))
if channel < 0 or channel > 1:
    raise Exception("Bad channel: {}".format(channel))


p = atomic_oven_controller.OvenPICInterface()

on_duty = 0.05
#on_duty = 0.01
dt = 0.05

p.set_pwm_duty(channel, 0)

p.adc_decimate(0)

p.adc_start_streaming(range(0,8))

time.sleep(dt)
p.set_pwm_duty(channel, on_duty)

time.sleep(dt)
p.set_pwm_duty(channel, 0)
time.sleep(dt)


results = p.adc_stop_streaming()[channel]

f = open("duty_impulse_response_data_{}.pkl".format(channel), "wb")
pickle.dump(results, f, -1)
