
import time

# import serial

# command_port="/dev/ttyO1"
# data_port="/dev/ttyO4"

# timeout=1
# sc = serial.Serial(
#     port=command_port,
#     baudrate=115200,
#     timeout=timeout
#     )

# while(1):
#     print(sc.readline())

import oven_pic_interface

p = oven_pic_interface.OvenPICInterface()
p._DEBUG = False


while(1):
    values = p.adc_read_calibrated_sample()
    print(values[1]["temperature"])

    values = p.adc_read_sample()
    temperature = values[6]*(2.5*(1000.0/40.0)*(1000.0/51.)) + 20
    print(temperature)
