
#import oven_pic_interface
import time
import pickle
import numpy as np

from artiq.protocols.pc_rpc import Client
p = Client("127.0.0.1", 5000, "atomic_oven_controller")

p.reset()

# calib = p.calibration_read_channel(1)
# calib["temperature_scale"] = -(2.5*(1000.0/40.0)*(1000.0/51.))
# p.calibration_set_channel(1, calib)
# p._read_calibrations()

#p = oven_pic_interface.OvenPICInterface()
#p._DEBUG = True

t_start = 1
t_on = 10
t_off = 5

temperature = 200
i_limit = 8

try:
    channel = 1
    p.set_pwm_duty(channel, 0)

    p.fb_set_limits("current", 0, 0.5, i_limit, 0)
    p.fb_set_limits("temperature", 0, np.sqrt(i_limit), 420, 400)


    p.fb_config("current", 0.008, 0.007, 0, 0)
    p.fb_set_setpoint("current", 0)
    p.fb_start("current")


    #p.fb_config("temperature", 0.08, 0.0004, -0.1, 9)
    p.fb_config("temperature", 0.08, 0.0008, -0.1, 9)
    p.fb_set_setpoint("temperature", temperature)
    p.adc_decimate(9)

    print(p.fb_read_status("temperature"))
    print(p.fb_read_status("current"))

    p.adc_start_streaming(list(range(0,8)))
    time.sleep(t_start)
    p.fb_start("temperature")
    time.sleep(t_on/2)
    print(p.fb_read_status("temperature"))
    time.sleep(t_on/2)

    print(p.fb_read_status("temperature"))
    print(p.fb_read_status("current"))

    p.fb_set_setpoint("temperature", 0)
    time.sleep(t_off)

    p.fb_stop("temperature")
    print(p.fb_read_status("current"))

    p.set_pwm_duty(channel, 0)



    results = p.adc_stop_streaming()[channel]

    f = open("measure_temperature_feedback_data.pkl", "wb")
    pickle.dump(results, f, -1)


finally:
    p.fb_set_setpoint("temperature", 0)
    p.set_pwm_duty(channel, 0)
