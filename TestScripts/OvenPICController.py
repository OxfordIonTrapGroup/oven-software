
import serial
import time
import numpy as np
import matplotlib 
matplotlib.use('Agg')
import matplotlib.pyplot as plt

import struct

class OvenPIC:
    _INS_MAGIC_START = 0xA5
    _INS_MAGIC_END   = 0x5A

    _CMD_ECHO                       = 0x00

    _CMD_PWM_SET_DUTY               = 0x10

    _CMD_ADC_STREAM                 = 0x20
    _CMD_ADC_DECIMATE               = 0x21
    _CMD_ADC_READ_LAST_CONVERSION   = 0x22

    _CMD_FEEDBACK_CONFIG            = 0x30
    _CMD_FEEDBACK_START             = 0x31
    _CMD_FEEDBACK_STOP              = 0x32
    _CMD_FEEDBACK_SETPOINT          = 0x33
    _CMD_FEEDBACK_READ_STATUS       = 0x34

    def __init__(self, port='/dev/ttyO5', baudrate=250000, timeout=1):
        self.s = serial.Serial(port=port, baudrate=baudrate, timeout=timeout)
        
        # Wake up the device
        self.s.write('x')
        
    def _send_command(self, command, data):
        packet = [self._INS_MAGIC_START]
        packet += [command]
        packet += [len(data)]
        packet += [0]
        packet += [self._INS_MAGIC_END]
        packet += data
        
        self.s.write(packet)
        
    def echo(self, data):
        self._send_command(self._CMD_ECHO, data)
        
        response = self.s.read(len(data))
        return response
    
################################
################### PWM 
##################################

    def set_duty(self, duty, read_response=True):
        period = (40000/200.0) - 1

        period = round(period)
        duty_int = round(duty*period)

        data = struct.pack('>H',duty_int)
        print([hex(ord(c)) for c in data])

        self._send_command(self._CMD_PWM_SET_DUTY, data)

        if read_response:
            response = self.s.read(1000)
        else:
            response = []
        return response

################################
################### ADC 
##################################
    def adc_decimate(self, decimation):
 
        data = struct.pack('>I',int(decimation))

        self._send_command(self._CMD_ADC_DECIMATE, data)

    def adc_read_last_conversion(self):
 
        self._send_command(self._CMD_ADC_READ_LAST_CONVERSION, [])
        response = self.s.read(1000)
        return response


################################
################### Feedback 
##################################

    def fb_start(self):
        self._send_command(self._CMD_FEEDBACK_START, [])

    def fb_stop(self):
        self._send_command(self._CMD_FEEDBACK_STOP, [])

    def fb_set_setpoint(self, setpoint):

        data = struct.pack('>i', setpoint)
        self._send_command(self._CMD_FEEDBACK_SETPOINT, data)

        #response = self.s.read(1000)
        #print(response)
        #print([hex(ord(c)) for c in response])

    def fb_read_status(self):

        self._send_command(self._CMD_FEEDBACK_READ_STATUS, [])
        response = self.s.read(1000)
        print(response)

    def fb_config(self, p, i, d):

        data = struct.pack('>3i', p, i, d)
        self._send_command(self._CMD_FEEDBACK_CONFIG, data)


##################################
#################### Streaming
###################################

    def stream_channels(self, channels, duration, block_size = 100):
        
        channel_byte = 0
        for channel in channels:
            channel_byte += 1 << channel
        print(channel_byte)
        start_time = time.time()
        # Start streaming
        self._send_command(self._CMD_STREAM_ADC, [channel_byte])
        
        now = start_time
        datas = []
        while(now < start_time + duration):
            datas.append(self.s.read(block_size))
            now = time.time()
            
        total_time = now - start_time
        self._send_command(self._CMD_STREAM_ADC, [0])

        array_datas = []
        for data in datas:
            array_datas.append( np.fromstring(data,dtype='uint8') )
            
        array_data = np.concatenate(array_datas)
        #print(type(array_data))
        print(array_data[:10])
        
        # If we stopped receiving half-way through a packet, clip it off
        l = len(array_data)
        unit_length = 1+4*len(channels)
        array_data = array_data[:unit_length*int(l/(unit_length))]
        
        #channel_bytes = array_data[::1+4*len(channels)]
        
        array_data = np.reshape(array_data, (-1,1+4*len(channels)) )
        channel_bytes = array_data[:,0]

        channel_arrays = []
        channel_values = []
        for i in range(len(channels)):
            channel_array = array_data[:,1+4*i:1+4*(i+1)]
            channel_array = channel_array.flatten()
            channel_array = np.frombuffer(channel_array, dtype='uint32')
            #print '[' + '\n'.join(hex(n) for n in channel_array) + ']'
            print(len(channel_array))
            
            channel_values.append(self._process_channel_data(channel_array))
            
        
        
        print(channel_bytes)
        print(total_time)
        print(len(array_data)/total_time)
        
        return channel_values

    def stream_channels_sequence(self, channels, sequence, block_size = 100, mode='duty'):
        
        # Sequence is a list of tuples with [(timeToChange, duty),...]
        lastTime = 0        
        for element in sequence:
            #print(element[0], lastTime)
            if element[0] < lastTime:

                raise Exception('Bad sequence time')

            lastTime = element[0]

        duration = lastTime

        channel_byte = 0
        for channel in channels:
            channel_byte += 1 << channel
        print(channel_byte)
        start_time = time.time()
        # Start streaming
        self._send_command(self._CMD_STREAM_ADC, [channel_byte])
        
        now = start_time
        datas = []
        nextSequenceIndex = 0
        while(now < start_time + duration):
            if( now >= start_time + sequence[nextSequenceIndex][0] ):
                if(mode == 'duty'):
                    self.set_duty( sequence[nextSequenceIndex][1], read_response=False )
                elif(mode == 'temp'):
                    setpoint = -(sequence[nextSequenceIndex][1] - 20.)*(51./1000.)*(40.0/1000.)*0x800000/2.5
                    self.fb_set_setpoint(setpoint)
                    print(setpoint)
                print(sequence[nextSequenceIndex])
                nextSequenceIndex += 1
                if nextSequenceIndex >= len(sequence):
                    break

            datas.append(self.s.read(block_size))
            now = time.time()
            
        total_time = now - start_time
        self._send_command(self._CMD_STREAM_ADC, [0])

        array_datas = []
        for data in datas:
            array_datas.append( np.fromstring(data,dtype='uint8') )
            
        array_data = np.concatenate(array_datas)
        #print(type(array_data))
        print(array_data[:10])
        
        # If we stopped receiving half-way through a packet, clip it off
        l = len(array_data)
        unit_length = 1+4*len(channels)
        array_data = array_data[:unit_length*int(l/(unit_length))]
        
        #channel_bytes = array_data[::1+4*len(channels)]
        
        array_data = np.reshape(array_data, (-1,1+4*len(channels)) )
        channel_bytes = array_data[:,0]

        channel_arrays = []
        channel_values = []
        for i in range(len(channels)):
            channel_array = array_data[:,1+4*i:1+4*(i+1)]
            channel_array = channel_array.flatten()
            channel_array = np.frombuffer(channel_array, dtype='uint32')
            #print '[' + '\n'.join(hex(n) for n in channel_array) + ']'
            print(len(channel_array))
            
            channel_values.append(self._process_channel_data(channel_array))
            
        
        
        print(channel_bytes)
        print(total_time)
        print(len(array_data)/total_time)
        
        return channel_values


    def _process_channel_data(self, channel_array):
    
        values = np.bitwise_and(channel_array, 0x7FFFFF).astype('int32')
        # Generate logical array of signs (+ = 0, - = 1)
        signs  = np.bitwise_and(channel_array, 1 << 23) != 0 
        
        values = np.where(signs, values - 0x800000, values)
        
        float_values = values*2.5/(0x800000*1.0)
        
        return float_values

        
p = OvenPIC()

response = p.echo('hi\n')
print(len(response))
print(response)

response = p.adc_read_last_conversion()
print(len(response))
print(response)

dd

#response = p.set_duty(0.08)
#print(len(response))
#print(response)


def test_streaming( duration = 10):
    
    channel_values = p.stream_channels([4,5,6,7], duration)    
    #channel_values = p.stream_channels([6], duration)
    #TC = ((-channel_values[0]*(1000.0/40.0)*(1000.0/51.)) + 20)
    
    VOut = 3*channel_values[0]
    IOven = 10.0*channel_values[1]
    TC = ((-channel_values[2]*(1000.0/40.0)*(1000.0/51.)) + 20)    
    VOven = -3*channel_values[3]
    
    #print(channel_values[0]*0x800000/2.5)
    
    t = np.linspace(0,duration,len(channel_values[0]))

    #plt.clf()
    #plt.plot(t, TC, '.')
    #plt.savefig('pp.pdf')
    #return
    plt.clf()
    fig, ax = plt.subplots(nrows=3, sharex=True)
    
    ax[0].plot(t, TC, '.')
    tAx = ax[0].twinx()
    tAx.plot(t,VOven/IOven, '.r')
    ax[0].set_ylabel('Temperature (C)')
    tAx.set_ylabel('Resistance (R)')
    
    ax[1].plot(t, IOven, '.')
    ax[1].set_ylabel('Current (A)')

    ax[2].plot(t, VOven, '.')
    #ax[2].plot(t, VOut, '.')
    ax[2].set_ylabel('Voltage (V)')
    
    ax[2].set_xlabel('Time (s)')
    #plt.tight_layout()
    plt.savefig('pp.pdf')

def test_streaming_sequence(mode='duty'):

    channel_values = p.stream_channels_sequence([4,5,6,7], [(0,20),(1,150),(10,20),(20,20)], mode=mode )       
    #channel_values = p.stream_channels_sequence([4,5,6,7], [(0,20),(1,150),(2,150)], mode=mode )    
    #channel_values = p.stream_channels_sequence([4,5,6,7], [(0,0),(2,0.08),(7,0.01),(10,0)] )
    
    VOut = 3*channel_values[0]
    IOven = 10.0*channel_values[1]
    TC = ((-channel_values[2]*(1000.0/40.0)*(1000.0/51.)) + 20)
    VOven = -3*channel_values[3]
    
    
    t = np.linspace(0,10,len(channel_values[0]))

    plt.clf()
    fig, ax = plt.subplots(nrows=3, sharex=True)
    
    ax[0].plot(t, TC, '.')
    tAx = ax[0].twinx()
    tAx.plot(t,VOven/IOven, '.r')
    tAx.set_ylim(0.2,0.3)
    ax[0].set_ylabel('Temperature (C)')
    tAx.set_ylabel('Resistance (R)')
    
    ax[1].plot(t, IOven, '.')
    ax[1].set_ylabel('Current (A)')

    ax[2].plot(t, VOven, '.')
    #ax[2].plot(t, VOut, '.')
    ax[2].set_ylabel('Voltage (V)')
    
    ax[2].set_xlabel('Time (s)')
    #plt.tight_layout()
    plt.savefig('pp.pdf')


#test_streaming_sequence()
#response = p.set_duty(0.0)
#print(len(response))

#######################################################
######### Feedback testing
#############################


def test_fb():
    T = 20
    setpoint = -(T - 20)*(51./1000.)*(40.0/1000.)*0x800000/2.5
    print(setpoint)
    p.fb_read_status()
    time.sleep(1)
    p.fb_config( -10, -1, 0 )
    time.sleep(1)
    p.fb_set_setpoint( setpoint )
    time.sleep(1)
    p.fb_start()

    #time.sleep(1)

    #for i in range(10):
    #    p.fb_read_status()
    #    time.sleep(0.1)


    response = p.s.read(1000)
    print(response)

    test_streaming_sequence(mode='temp')
    response = p.s.read(100000)
    p.fb_read_status()

    #time.sleep(0.5)
    p.fb_read_status()
    p.fb_stop()

test_fb()
#########

def timing_test():
    print('Start')
    p.fb_start()
    print('Stop')
    p.fb_stop()
    p.fb_start()
    time.sleep(1)
    print('Read 1')
    p.fb_read_status()
    time.sleep(1)
    print('Read 2')
    p.fb_read_status()


