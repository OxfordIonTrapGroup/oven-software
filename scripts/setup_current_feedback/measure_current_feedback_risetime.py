

import time
import pickle
import atomic_oven_controller

channel = int(input("Oven channel to test (0,1): "))
if channel < 0 or channel > 1:
    raise Exception("Bad channel: {}".format(channel))

p = atomic_oven_controller.OvenPICInterface()
controller_name = "current_{}".format(channel)



dt = 0.05

p.set_pwm_duty(channel, 0)


print(p.fb_read_status(controller_name))


current_limits = {}
current_limits['cv_min'] = 0
current_limits['cv_max'] = 0.1
current_limits['value_max'] = 8
current_limits['setpoint_slewrate'] = 0
p.fb_set_limits(controller_name, current_limits)


current_config = {}
current_config['p'] = 0.008
current_config['i'] = 0.007
current_config['d'] = 0
current_config['sample_decimation'] = 0 # 1000 Hz update rate

p.fb_set_config(controller_name, current_config)
p.fb_set_setpoint(controller_name, 1)
p.adc_decimate(0)

print(p.fb_read_status(controller_name))

p.adc_start_streaming(range(0,8))
time.sleep(dt)
p.fb_start(controller_name)

print(p.fb_read_status(controller_name))

time.sleep(dt)
print(p.fb_read_status(controller_name))
p.fb_set_setpoint(controller_name, 0)
time.sleep(dt)



p.fb_stop(controller_name)
print(p.fb_read_status(controller_name))

p.set_pwm_duty(channel, 0)

p.safety_status()

#samples = p.adc_stop_streaming()
results = p.adc_stop_streaming()[channel]


#results = oven_pic_interface.convert_samples(channel, samples)

f = open("current_feedback_risetime_data_{}.pkl".format(channel), "wb")
pickle.dump(results, f, -1)
f.close()

