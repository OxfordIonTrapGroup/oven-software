
import time
import atomic_oven_controller

p = atomic_oven_controller.OvenPICInterface(timeout=2)

# Set the maximum duty cycle to be 2%, which should be safe in all cases
p.safety_set_channel(0, "duty_max", 0.02)
p.safety_set_channel(1, "duty_max", 0.02)
print("Setting maximum duty cycle to 2\% on both channels")

# Save the settings
p.settings_save()



