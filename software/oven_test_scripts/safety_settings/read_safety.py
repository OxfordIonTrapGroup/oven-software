
import time
import atomic_oven_controller


channel = int(input("Oven channel to test (0,1): "))
if channel < 0 or channel > 1:
    raise Exception("Bad channel: {}".format(channel))


p = atomic_oven_controller.OvenPICInterface(timeout=2)

safety_settings = p.safety_read_channel(channel)

print("Safety settings for channel {}:".format(channel))
print("===============================")

for key in sorted(safety_settings.keys()):
    print(key, ":")
    print(safety_settings[key])

