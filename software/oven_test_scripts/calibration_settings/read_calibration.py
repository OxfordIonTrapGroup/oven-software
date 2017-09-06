
import time
import atomic_oven_controller


channel = int(input("Oven channel to test (0,1): "))
if channel < 0 or channel > 1:
    raise Exception("Bad channel: {}".format(channel))


p = atomic_oven_controller.OvenPICInterface(timeout=2)

calib = p.calibration_read_channel(channel)

print("Calibration values for channel {}:".format(channel))
print("==================================")

for key in sorted(calib.keys()):
    print(key, ":")
    print(calib[key])

