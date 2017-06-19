
import oven_pic_interface
import time
import pickle

p = oven_pic_interface.OvenPICInterface()
p._DEBUG = False
#p.settings_set_to_factory()

calib = p.calibration_read_channel(1)
print(calib)
# #calib["temperature_current_coefficient"] = -4.9+0.3
# p.calibration_set_channel(1, calib)

p._read_calibrations()

dt = 0.05

channel = 1
p.set_pwm_duty(channel, 0)

print(p.fb_read_status("current"))

p.fb_set_limits("current", 0, 0.2, 6)


p.fb_config("current", 0.008, 0.007, 0)
p.fb_set_setpoint("current", 4)
p.adc_decimate(0)

print(p.fb_read_status("current"))

p.adc_start_streaming(range(0,8))
time.sleep(dt)
p.fb_start("current")

print(p.fb_read_status("current"))

time.sleep(dt)
print(p.fb_read_status("current"))
p.fb_set_setpoint("current", 0)
time.sleep(dt)



p.fb_stop("current")
print(p.fb_read_status("current"))

p.set_pwm_duty(channel, 0)

#samples = p.adc_stop_streaming()
results = p.adc_stop_streaming()[channel]


#results = oven_pic_interface.convert_samples(channel, samples)

f = open("measure_current_feedback_risetime_data.pkl", "wb")
pickle.dump(results, f, -1)
