
#import oven_pic_interface
import time
import pickle

from artiq.protocols.pc_rpc import Client
p = Client("127.0.0.1", 5000, "atomic_oven_controller")

p.reset()
#p = oven_pic_interface.OvenPICInterface()
#p._DEBUG = True

t_start = 1
t_on = 5
t_off = 5

temperature = 350


try:
    channel = 1
    p.set_pwm_duty(channel, 0)

    p.fb_set_limits("current", 0, 0.25, 9)
    p.fb_set_limits("temperature", 0, 9, 420)


    p.fb_config("current", 0.008, 0.007, 0)
    p.fb_set_setpoint("current", 0)
    p.fb_start("current")


    #p.fb_config("temperature", 0.04, 0.00008, 0)
    p.fb_config("temperature", 0.015, 0.000004, 0)
    p.fb_set_setpoint("temperature", temperature)
    p.adc_decimate(9)

    print(p.fb_read_status("temperature"))
    print(p.fb_read_status("current"))

    p.adc_start_streaming(list(range(0,8)))
    time.sleep(t_start)
    p.fb_start("temperature")
    time.sleep(t_on)

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
