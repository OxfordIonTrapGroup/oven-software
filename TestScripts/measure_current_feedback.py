
import oven_pic_interface
import time
import pickle

p = oven_pic_interface.OvenPICInterface()

dt = 1

channel = 1
p.set_pwm_duty(channel, 0)


p.fb_config(0.01, 0.01, 0)
p.fb_set_setpoint(1.5)
p.adc_decimate(0)

print(p.fb_read_status())

p.adc_start_streaming(range(0,8))
time.sleep(dt)
p.fb_start()

print(p.fb_read_status())

time.sleep(dt)
print(p.fb_read_status())
time.sleep(dt)
print(p.fb_read_status())

p.fb_stop()
print(p.fb_read_status())

time.sleep(dt)
print(p.fb_read_status())

p.set_pwm_duty(channel, 0)

samples = p.adc_stop_streaming()



results = oven_pic_interface.convert_samples(channel, samples)

f = open("measure_current_feedback_data.pkl", "wb")
pickle.dump(results, f, -1)
