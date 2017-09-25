
import time
import atomic_oven_controller


channel = int(input("Oven channel to set current controller settings of (0,1): "))
if channel < 0 or channel > 1:
    raise Exception("Bad channel: {}".format(channel))


p = atomic_oven_controller.OvenPICInterface(timeout=2)
controller_name = "current_{}".format(channel)


old_config = p.fb_get_config(controller_name)
old_limits = p.fb_get_limits(controller_name)

print("Old config: ", old_config)
print("Old limits: ", old_limits)


# PID gains should work for most ovens
# TGB 09/2017
new_config = {}
new_config['p'] = 0.008
new_config['i'] = 0.007
new_config['d'] = 0
new_config['sample_decimation'] = 0

new_limits = {}
new_limits['cv_min'] = 0
new_limits['cv_max'] = 0.1 # 10% max duty cycle
new_limits['value_max'] = 8 # 8A max current
new_limits['setpoint_slewrate'] = 0 # No slew rate limit

p.fb_set_config(controller_name, new_config)
p.fb_set_limits(controller_name, new_limits)

set_config = p.fb_get_config(controller_name)
set_limits = p.fb_get_limits(controller_name)

print("New config: ", set_config)
print("New limits: ", set_limits)

print("Saving...")
p.settings_save()