
import time
import numpy as np

from OvenPICController import OvenPIC

def TC(T):
    return -(T - 20)*(51./1000.)*(40.0/1000.)*0x800000/2.5

def IC(I):
    DD = (I/10.0)*0x800000/2.5
    print(DD)
    return DD

p = OvenPIC()
p.fb_stop()

def test_streaming_fixed():

    p.adc_decimate(0)
    p.adc_start_streaming([4,5,6,7])
    p.adc_poll_stream(2)

    p.pwm_set_duty(0.1)
    p.adc_poll_stream(2)
    p.pwm_set_duty(0.095)
    p.adc_poll_stream(2)
    p.pwm_set_duty(0.09)

    p.adc_poll_stream(2)
    p.pwm_set_duty(0.085)

    p.adc_poll_stream(2)
    p.pwm_set_duty(0.08)

    p.adc_poll_stream(2)
    p.pwm_set_duty(0)
    data = p.adc_stop_streaming()

    print(len(data[6]))

    np.savetxt('data.txt', np.transpose(data[4:8])) 

def test_fb():
    p.fb_config( -10, -2, 0 )
    p.fb_set_setpoint( TC(20) )
    p.fb_set_limits(IC(10), TC(200))
    p.fb_start()

    p.adc_decimate(0)
    p.adc_start_streaming([4,5,6,7])
    p.adc_poll_stream(2)

    p.fb_set_setpoint( TC(300) )
    p.adc_poll_stream(20)
    p.fb_set_setpoint( TC(20) )
    p.adc_poll_stream(10)

    p.fb_stop()
    data = p.adc_stop_streaming()

    print(len(data[6]))
    np.savetxt('data.txt', np.transpose(data[4:8])) 

def test_fb_current():
    p.fb_config( 2, 32, 0 )
    p.fb_set_setpoint( IC(0) )
    p.fb_set_limits(IC(10), TC(200))
    p.fb_start()

    p.adc_decimate(0)
    p.adc_start_streaming([4,5,6,7])
    p.adc_poll_stream(2)

    p.fb_set_setpoint( IC(1) )
    p.adc_poll_stream(10)
    p.fb_set_setpoint( IC(0) )
    p.adc_poll_stream(5)

    p.fb_stop()
    data = p.adc_stop_streaming()

    print(len(data[6]))
    np.savetxt('data.txt', np.transpose(data[4:8])) 

#test_streaming_fixed()

test_fb_current()


