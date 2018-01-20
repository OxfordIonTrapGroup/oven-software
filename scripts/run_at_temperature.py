import time
import numpy as np
import pickle
import atomic_oven_controller
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("channel", type=int)
parser.add_argument("--temperature", type=float, default=80.0)
parser.add_argument("--duration", type=float, default=20)
args = parser.parse_args()


channel = args.channel
assert channel in [0,1]

p = atomic_oven_controller.OvenPICInterface(timeout=2)
current_controller = "current_{}".format(channel)
temperature_controller = "temperature_{}".format(channel)

poll_time = 0.1


try:
    current_limits = p.fb_get_limits(current_controller)
    current_limits['value_max'] = 3.95
    p.fb_set_limits(current_controller, current_limits)


    p.fb_set_setpoint(current_controller, 0)
    p.fb_start(current_controller)

    p.fb_set_setpoint(temperature_controller, args.temperature)

    print(p.fb_read_status(temperature_controller))
    print(p.fb_read_status(current_controller))

    start_time = time.time()

    p.fb_start(temperature_controller)

    while time.time() < start_time + args.duration:
        values = p.adc_read_calibrated_sample()
        print("{:0.2f} C, {:0.2f} A".format(values[channel]["temperature"], values[channel]["current"]))
        time.sleep(poll_time)

    p.fb_stop(temperature_controller)

    p.fb_set_setpoint(current_controller, 0)
    p.fb_set_setpoint(temperature_controller, 0)
    p.set_pwm_duty(channel, 0)

    print(p.fb_read_status(temperature_controller))
    print(p.fb_read_status(current_controller))

    time.sleep(1)

    print(p.fb_read_status(temperature_controller))


finally:
    try:
        p.fb_stop(temperature_controller)

        p.fb_set_setpoint(temperature_controller, 0)
        p.fb_set_setpoint(current_controller, 0)
        p.set_pwm_duty(channel, 0)
    finally:
        p.fb_stop(temperature_controller)

        p.fb_set_setpoint(temperature_controller, 0)
        p.fb_set_setpoint(current_controller, 0)
        p.set_pwm_duty(channel, 0)
        print("Oven duty set to 0")
        time.sleep(1)
        values = p.adc_read_calibrated_sample()
        print("{:0.2f} C, {:0.2f} A".format(values[channel]["temperature"], values[channel]["current"]))
