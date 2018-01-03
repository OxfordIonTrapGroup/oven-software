
import time
import atomic_oven_controller


channel = int(input("Oven channel to set temperature-current compensation of (0,1): "))
if channel < 0 or channel > 1:
    raise Exception("Bad channel: {}".format(channel))


p = atomic_oven_controller.OvenPICInterface(timeout=2)

calib = p.calibration_read_channel(channel)
print("Previous value (C/A): ", calib["temperature_current_coefficient"])

new_value = float(input("Enter new value (C/A): "))

calib["temperature_current_coefficient"] = new_value
print("New value = ", calib["temperature_current_coefficient"])
print("Saving...")

p.calibration_set_channel(channel, calib)
p.settings_save()