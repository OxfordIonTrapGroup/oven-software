
import time
import atomic_oven_controller

channel = int(input("Oven channel to set (0,1): "))
if channel < 0 or channel > 1:
    raise Exception("Bad channel: {}".format(channel))

p = atomic_oven_controller.OvenPICInterface(timeout=2)


p.safety_set_channel(channel, "oven_temperature_min", -50)
print("Setting minimum temperature to -50C on channel", channel)


# Save the settings
p.settings_save()



