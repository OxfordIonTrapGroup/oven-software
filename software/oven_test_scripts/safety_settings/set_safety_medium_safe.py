
import time
import atomic_oven_controller

p = atomic_oven_controller.OvenPICInterface(timeout=2)

# Set the maximum duty cycle to be 5%, which should be safe in all cases
p.safety_set_channel(0, "duty_max", 0.05)
p.safety_set_channel(1, "duty_max", 0.05)
print("Setting maximum duty cycle to 5\% on both channels")

# Save the settings
p.settings_save()



