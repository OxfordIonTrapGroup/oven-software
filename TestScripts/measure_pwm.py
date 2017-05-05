
import oven_pic_interface
import time
import pickle

p = oven_pic_interface.OvenPICInterface()

dt = 0.5

channel = 1

p.adc_decimate(0)

p.adc_start_streaming(range(0,8))
time.sleep(dt)
p.set_pwm_duty(channel, 0.1)
time.sleep(dt)
p.set_pwm_duty(channel, 0)
time.sleep(dt)

samples = p.adc_stop_streaming()

results = oven_pic_interface.convert_samples(channel, samples)

f = open("measure_pwm_data.pkl", "wb")
pickle.dump(results, f, -1)
