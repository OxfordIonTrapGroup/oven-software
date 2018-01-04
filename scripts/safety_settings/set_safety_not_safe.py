
import time
import atomic_oven_controller

channel = int(input("Oven channel to set (0,1): "))
if channel < 0 or channel > 1:
    raise Exception("Bad channel: {}".format(channel))

p = atomic_oven_controller.OvenPICInterface(timeout=2)

# Set the maximum duty cycle to be 10%
p.safety_set_channel(channel, "duty_max", 0.10)
print("Setting maximum duty cycle to 10% on channel", channel)

p.safety_set_channel(channel, "oven_temperature_max", 500)
print("Setting maximum temperature to 500C on channel", channel)


# Save the settings
p.settings_save()



