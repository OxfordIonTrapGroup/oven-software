
import time

import atomic_oven_controller

p = atomic_oven_controller.Interface()
p._DEBUG = False

while(1):
    values = p.adc_read_calibrated_sample()
    print("{:0.2f}, {:0.2f}".format(values[0]["temperature"], values[1]["temperature"]))
    time.sleep(0.1)
