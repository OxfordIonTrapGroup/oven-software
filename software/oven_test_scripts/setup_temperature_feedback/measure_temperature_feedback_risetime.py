
import time
import numpy as np
import pickle
import atomic_oven_controller

channel = int(input("Oven channel to set current controller settings of (0,1): "))
if channel < 0 or channel > 1:
    raise Exception("Bad channel: {}".format(channel))


p = atomic_oven_controller.OvenPICInterface(timeout=2)
current_controller = "current_{}".format(channel)
temperature_controller = "temperature_{}".format(channel)

t_start = 1
t_on = 10
t_off = 5

t_on = 20
t_off = 10

# Temperature to regulate to
test_setpoint = 150

try:

    current_limits = p.fb_get_limits(current_controller)

    p.fb_set_setpoint(current_controller, 0)
    p.fb_start(current_controller)


    temperature_limits = {}
    temperature_limits['cv_min'] = 0
    temperature_limits['cv_max'] = np.sqrt(current_limits['value_max'])
    temperature_limits['value_max'] = 450
    temperature_limits['setpoint_slewrate'] = 400 # C/s
    p.fb_set_limits(temperature_controller, temperature_limits)

    temperature_config = {}
    temperature_config['p'] = 0.08
    temperature_config['i'] = 0.0008
    temperature_config['d'] = -0.1
    temperature_config['sample_decimation'] = 9 # 100 Hz update rate

    p.fb_set_config(temperature_controller, temperature_config)
    p.fb_set_setpoint(temperature_controller, test_setpoint)

    # Set up the ADC decimation to reduce data rate by factor of 10
    p.adc_decimate(9)

    print(p.fb_read_status(temperature_controller))
    print(p.fb_read_status(current_controller))

    p.adc_start_streaming(list(range(0,8)))
    time.sleep(t_start)
    p.fb_start(temperature_controller)
    time.sleep(t_on/2)
    print(p.fb_read_status(temperature_controller))
    time.sleep(t_on/2)

    print(p.fb_read_status(temperature_controller))
    print(p.fb_read_status(current_controller))

    p.fb_set_setpoint(temperature_controller, 0)
    time.sleep(t_off)

    p.fb_stop(temperature_controller)
    print(p.fb_read_status(current_controller))

    results = p.adc_stop_streaming()[channel]

    f = open("temperature_feedback_risetime_data_{}.pkl".format(channel), "wb")
    pickle.dump(results, f, -1)


finally:
    p.fb_set_setpoint(temperature_controller, 0)
    p.fb_set_setpoint(current_controller, 0)
    p.set_pwm_duty(channel, 0)

