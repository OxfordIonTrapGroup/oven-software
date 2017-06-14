
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

p = oven_pic_interface.OvenPICInterface(timeout=2)
p._DEBUG = True

p.settings_set_to_factory()

safety_settings = p.safety_read_channel(1)
print(safety_settings)

#p.safety_set_channel(1, "duty_max", 0.4)

#p.settings_save()

safety_settings = p.safety_read_channel(1)
print(safety_settings)