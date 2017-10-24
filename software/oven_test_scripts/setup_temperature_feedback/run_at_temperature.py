
import time
import numpy as np
import pickle
import atomic_oven_controller

# channel = int(input("Oven channel to set current controller settings of (0,1): "))
# if channel < 0 or channel > 1:
#     raise Exception("Bad channel: {}".format(channel))

channel = 1


p = atomic_oven_controller.OvenPICInterface(timeout=2)
current_controller = "current_{}".format(channel)
temperature_controller = "temperature_{}".format(channel)

t_on = 60*5
poll_time = 0.1


# Temperature to regulate to
test_setpoint = 300 # Sr trapping 
# test_setpoint = 320 # Sr neutral fluor 

try:

    current_limits = p.fb_get_limits(current_controller)
    current_limits['value_max'] = 3.95
    p.fb_set_limits(current_controller, current_limits)


    p.fb_set_setpoint(current_controller, 0)
    p.fb_start(current_controller)


    temperature_limits = {}
    temperature_limits['cv_min'] = 0
    temperature_limits['cv_max'] = np.sqrt(current_limits['value_max'])
    temperature_limits['value_max'] = 350
    temperature_limits['setpoint_slewrate'] = 100 # C/s
    p.fb_set_limits(temperature_controller, temperature_limits)

    temperature_config = {}
    temperature_config['p'] = 0.08
    temperature_config['i'] = 0.0008
    temperature_config['d'] = -0.1
    temperature_config['sample_decimation'] = 9 # 100 Hz update rate

    p.fb_set_config(temperature_controller, temperature_config)
    p.fb_set_setpoint(temperature_controller, test_setpoint)

    print(p.fb_read_status(temperature_controller))
    print(p.fb_read_status(current_controller))

    start_time = time.time()

    p.fb_start(temperature_controller)

    while time.time() < start_time + t_on:

        values = p.adc_read_calibrated_sample()
        print("{:0.2f} C, {:0.2f} A".format(values[channel]["temperature"], values[channel]["current"]))
        #print(p.fb_read_status(temperature_controller), end='')
        #print(p.fb_read_status(current_controller), end='')
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
