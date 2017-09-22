
import time
import atomic_oven_controller


channel = int(input("Oven channel to reverse tc polarity of (0,1): "))
if channel < 0 or channel > 1:
    raise Exception("Bad channel: {}".format(channel))


p = atomic_oven_controller.OvenPICInterface(timeout=2)

calib = p.calibration_read_channel(channel)
print("Switching TC scale from ", calib["temperature_scale"])
calib["temperature_scale"] =-calib["temperature_scale"]
print("... to ", calib["temperature_scale"])
p.calibration_set_channel(channel, calib)

p.settings_save()