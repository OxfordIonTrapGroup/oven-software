
import oven_pic_interface

def TCCal(data):
    TC = ((-data*(1000.0/40.0)*(1000.0/51.)) + 20) 
    return TC

p = oven_pic_interface.OvenPICInterface()

a = oven_pic_interface.ADC_CHANNELS


while(1):
    data = p.adc_read_sample()
    V = data[a["T"][1]]

    V = TCCal(V)

    print(V, data[-1])
    
