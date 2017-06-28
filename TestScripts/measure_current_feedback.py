
import oven_pic_interface
import time
import pickle

p = oven_pic_interface.OvenPICInterface()
p._DEBUG = False

t_start = 1
t_on = 20
t_off = 10

I_on = 1

channel = 1
p.set_pwm_duty(channel, 0)

print(p.fb_read_status("current"))

p.fb_set_limits("current", 0, 0.1, 4)

p.fb_config("current", 0.008, 0.007, 0)
p.fb_set_setpoint("current", I_on)
p.adc_decimate(9)

print(p.fb_read_status("current"))

p.adc_start_streaming(range(0,8))

time.sleep(t_start)

p.fb_start("current")

print(p.fb_read_status("current"))

time.sleep(t_on)
print(p.fb_read_status("current"))
p.fb_set_setpoint("current", 0)

time.sleep(t_off)

p.fb_stop("current")
print(p.fb_read_status("current"))

p.set_pwm_duty(channel, 0)

samples = p.adc_stop_streaming()



results = oven_pic_interface.convert_samples(channel, samples)

f = open("measure_current_feedback_data.pkl", "wb")
pickle.dump(results, f, -1)
