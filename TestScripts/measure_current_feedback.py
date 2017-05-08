
import oven_pic_interface
import time
import pickle

p = oven_pic_interface.OvenPICInterface()
p._DEBUG = False
dt = 1

channel = 1
p.set_pwm_duty(channel, 0)

print(p.fb_read_status("current"))



p.fb_config("current", 0.01, 0.01, 0)
p.fb_set_setpoint("current", 1.5)
p.adc_decimate(0)

print(p.fb_read_status("current"))

p.adc_start_streaming(range(0,8))
time.sleep(dt)
p.fb_start("current")

print(p.fb_read_status("current"))

time.sleep(dt)
print(p.fb_read_status("current"))
time.sleep(dt)
print(p.fb_read_status("current"))

p.fb_stop("current")
print(p.fb_read_status("current"))

time.sleep(dt)
print(p.fb_read_status("current"))

p.set_pwm_duty(channel, 0)

samples = p.adc_stop_streaming()



results = oven_pic_interface.convert_samples(channel, samples)

f = open("measure_current_feedback_data.pkl", "wb")
pickle.dump(results, f, -1)
