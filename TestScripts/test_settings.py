
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
#print(p.adc_read_calibrated_sample())

#p.settings_print()

#p.settings_set_to_factory()

calib = p.calibration_read_channel(1)
print(calib)

# calib["oven_voltage_scale"] = 10

calib["temperature_current_coefficient"] =-4.9+0.3
p.calibration_set_channel(1, calib)

calib = p.calibration_read_channel(1)
print(calib)

# p.settings_print()

#p.settings_save()

# # p.settings_print()

# # p.settings_load()
# time.sleep(2)
print(p.adc_read_calibrated_sample())
