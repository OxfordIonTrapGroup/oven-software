
import oven_pic_interface
import time
import pickle

p = oven_pic_interface.OvenPICInterface()

t_start = 1
t_on = 2
t_off = 2

channel = 1
p.set_pwm_duty(channel, 0)

p.fb_config("current", 0.008, 0.007, 0)
p.fb_set_setpoint("current", 0)
p.fb_start("current")


p.fb_config("temperature", 0.04, 0.00006, 0)
p.fb_set_setpoint("temperature", 400)
p.adc_decimate(9)

print(p.fb_read_status("temperature"))
print(p.fb_read_status("current"))

p.adc_start_streaming(range(0,8))
time.sleep(t_start)
p.fb_start("temperature")
time.sleep(t_on)

print(p.fb_read_status("temperature"))
print(p.fb_read_status("current"))

p.fb_set_setpoint("temperature", 0)
time.sleep(t_off)

p.fb_stop("temperature")
print(p.fb_read_status("current"))

p.set_pwm_duty(channel, 0)

samples = p.adc_stop_streaming()



results = oven_pic_interface.convert_samples(channel, samples)

f = open("measure_temperature_feedback_data.pkl", "wb")
pickle.dump(results, f, -1)
