
#import oven_pic_interface
import time
import pickle

from artiq.protocols.pc_rpc import Client
p = Client("127.0.0.1", 5000, "atomic_oven_controller")

p.reset()
#p = oven_pic_interface.OvenPICInterface()
#p._DEBUG = True


temperature = 410
t_on_max = 100

channel = 1

try:

    p.set_pwm_duty(channel, 0)

    p.fb_set_limits("current", 0, 0.25, 9)
    p.fb_set_limits("temperature", 0, 9, 420)


    p.fb_config("current", 0.008, 0.007, 0)
    p.fb_set_setpoint("current", 0)
    p.fb_start("current")


    #p.fb_config("temperature", 0.04, 0.00008, 0)
    p.fb_config("temperature", 0.015, 0.000008, 0)
    p.fb_set_setpoint("temperature", temperature)

    print(p.fb_read_status("temperature"))
    print(p.fb_read_status("current"))

    p.fb_start("temperature")

    print(p.fb_read_status("temperature"))
    print(p.fb_read_status("current"))

    time.sleep(t_on_max)

finally:
    p.fb_set_setpoint("temperature", 0)

    p.fb_stop("temperature")
    print(p.fb_read_status("current"))

    p.set_pwm_duty(channel, 0)
