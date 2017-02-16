
import serial
import time
import numpy as np
#import matplotlib 
#matplotlib.use('Agg')
#import matplotlib.pyplot as plt

import struct

class OvenPIC:
    _INS_MAGIC_START = 0xA5
    _INS_MAGIC_END   = 0x5A

    _INS_HEADER_LEN  = 5

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

    _CMD_ERROR                      = 0xFF

    def __init__(self, port='/dev/ttyO5', baudrate=250000, timeout=1):
        self.s = serial.Serial(
            port=port, baudrate=baudrate, 
            timeout=timeout)
        
        self.streaming = False
        self.rx_buffer = []

        # Wake up the device
        self.s.write(b'x')
        
    def _send_command(self, command, data):
        packet = [self._INS_MAGIC_START]
        packet += [command]
        packet += [len(data)]
        packet += [0]
        packet += [self._INS_MAGIC_END]
        packet += data
        
        self.s.write(packet)

    def _read(self, command=None, timeout=5):
        """Block until a packet is found. If command is not none, 
        then throws an exception if a different command is found"""

        if self.streaming:
            raise Exception('_read called whilst device is streaming')

        start_time = time.time()
        now = 0
        while(now < timeout):
            response = self._read_packet()
            if response is None:

                time.sleep(0.1)
                now = time.time() - start_time
                continue

            rx_command, data = response
            if rx_command is self._CMD_ERROR:
                raise Exception('Received error: ' + data.encode('ascii'))

            if command is None:
                return data

            if rx_command is command:
                return data
            else:
                raise Exception('Received reply with wrong command code: '
                    '{0} vs {1}'.format(hex(rx_command), hex(command)) )

        raise Exception('Read timed out')


    def _read_packet(self):
        """Read the next message from the PIC"""

        # Read in any new data
        if (self.s.inWaiting() > 0 ): 
            new_data = self.s.read(self.s.inWaiting())
            self.rx_buffer += new_data

        if len(self.rx_buffer) == 0:
            return None


        # Find first magic byte
        i_start = 0
        while self.rx_buffer[i_start] != self._INS_MAGIC_START:
            i_start += 1
            if i_start >= len(self.rx_buffer):
                # If no start packet was found, invalidate the buffer 
                # and return
                self.rx_buffer = []
                return None

        # Now check that there is enough data in the buffer for this packet
        if i_start + self._INS_HEADER_LEN > len(self.rx_buffer):
            # If the packet is not fully there, discard the start of
            # the buffer and return
            self.rx_buffer = self.rx_buffer[i_start:]
            return None

        # Now we have a valid start symbol and enough length to see 
        # the header
        magic_start, command, length, crc, magic_end = \
            struct.unpack('>5B', 
                bytes(self.rx_buffer[i_start:i_start+self._INS_HEADER_LEN]) )


        if magic_end != self._INS_MAGIC_END:
            print('Bad magic end byte')
            # Discard the packet and return
            self.rx_buffer = self.rx_buffer[i_start+self._INS_HEADER_LEN:]
            return None

        if i_start + self._INS_HEADER_LEN + length > len(self.rx_buffer):
            # If there is not enough data in the buffer yet, rebase 
            # and return
            self.rx_buffer = self.rx_buffer[i_start:]
            return None

        data = bytes(self.rx_buffer[
            i_start+self._INS_HEADER_LEN :
            i_start+self._INS_HEADER_LEN + length
            ] )
        self.rx_buffer = self.rx_buffer[
            i_start+self._INS_HEADER_LEN + length :
            ]

        return command, data


    def echo(self, data):
        if type(data) is str:
            data = data.encode('ascii')

        self._send_command(self._CMD_ECHO, data)
        
        data = self._read()
        return data
    
################################
################### PWM 
##################################

    def pwm_set_duty(self, duty, read_response=True):
        period = (40000/200.0) - 1

        period = round(period)
        duty_int = round(duty*period)

        print(duty_int)

        data = struct.pack('<H',duty_int)

        self._send_command(self._CMD_PWM_SET_DUTY, data)


################################
################### ADC 
##################################
    def adc_decimate(self, decimation):
 
        data = struct.pack('<I',int(decimation))

        self._send_command(self._CMD_ADC_DECIMATE, data)

    def adc_read_last_conversion(self):
 
        self._send_command(self._CMD_ADC_READ_LAST_CONVERSION, [])

        data = self._read()
        samples = struct.unpack('<8i', data)

        return samples
            
    def adc_start_streaming(self, channels):

        if self.streaming:
            raise Exception(
                'start_streaming called when streaming already active')



        channel_byte = 0
        safe_channels = []
        for i in range(len(channels)):
            channel = channels[i]
            if not 0 < channel < 8:
                raise Exception(
                    'Requested channel out of range: {0}'.format(channel))
            if channel not in safe_channels:
                safe_channels.append(channel)
                channel_byte |= 1 << channel

        # Clear the rx buffer
        if self.s.inWaiting() > 0:
            self.s.read(self.s.inWaiting())
        self.rx_buffer = []

        self.streaming_start_time = time.time()
        self.streaming_channels = safe_channels
        self.streaming_channel_byte = channel_byte
        self.streaming_buffers = []
        self.streaming = True
        # Start streaming
        self._send_command(self._CMD_ADC_STREAM, [channel_byte])

    def adc_stop_streaming(self, read_delay=0.5):

        if not self.streaming:
            raise Exception(
                'stop_streaming called when streaming not active')

        # Send the stop command
        self._send_command(self._CMD_ADC_STREAM, [0])

        self.streaming_stop_time = time.time()

        # Sleep to allow the data to finish being sent
        time.sleep(read_delay)

        # Poll the stream one final time
        self.adc_poll_stream()

        self.streaming = False

        # Now cat the data buffers together
        if len(self.streaming_buffers) > 1:
            array_data = np.concatenate([
                np.frombuffer(b, dtype='uint8') 
                for b in self.streaming_buffers])
        elif len(self.streaming_buffers) == 1:
            array_data = np.frombuffer(self.streaming_buffers[0], dtype='uint8')
        else:
            raise Exception('No data received in adc stream')

        # If we stopped receiving half-way through a packet, clip it off
        unit_length = 1+4*len(self.streaming_channels)
        array_data = array_data[:unit_length*int(len(array_data)/unit_length)]

  
        # Reshape the array so that the first index references the 
        # sample number
        array_data = np.reshape(array_data, (-1, 
            1+4*len(self.streaming_channels)))
        channel_bytes = array_data[:,0]

        # Check that the channel_bytes are all the same
        if np.sum(channel_bytes != self.streaming_channel_byte) != 0:
            # If there are any samples which have the wrong byte
            raise Exception(
                'Bad channel_bytes in adc_streamed data, sync error')

        channel_arrays = []
        channel_values = []
        for i in range(len(self.streaming_channels)):
            channel_array = array_data[:,1+4*i:1+4*(i+1)]
            channel_array = channel_array.flatten()
            channel_array = np.frombuffer(channel_array, dtype='uint32')

            channel_values.append(self._process_channel_data(channel_array))

        # Now sort the data into a list indexed by the channel number
        all_channel_values = []
        channel_values_index = 0

        for i in range(8):
            if i in self.streaming_channels:
                all_channel_values.append(channel_values[channel_values_index])
                channel_values_index += 1
            else:
                all_channel_values.append([])

        return all_channel_values 

    def adc_poll_stream(self, duration=0):
        """Call this semi-regularly during streaming mode to empty the
        receive buffer"""


        if not self.streaming:
            raise Exception(
                'stop_streaming called when streaming not active')

        start_time = time.time()
        now = 0
        while(now <= duration):
            if self.s.inWaiting() > 0:
                self.streaming_buffers.append(self.s.read(
                    self.s.inWaiting()))
            time.sleep(0.1)
            now = time.time() - start_time


################################
################### Feedback 
##################################

    def fb_start(self):
        self._send_command(self._CMD_FEEDBACK_START, [])

    def fb_stop(self):
        self._send_command(self._CMD_FEEDBACK_STOP, [])

    def fb_set_setpoint(self, setpoint):

        data = struct.pack('<i', int(setpoint))
        self._send_command(self._CMD_FEEDBACK_SETPOINT, data)

        #response = self.s.read(1000)
        #print(response)
        #print([hex(ord(c)) for c in response])

    def fb_read_status(self):

        # self._send_command(self._CMD_FEEDBACK_READ_STATUS, [])
        # response = self.s.read(1000)
        # print(response)

        self._send_command(self._CMD_FEEDBACK_READ_STATUS, [])
        data = self._read(command=self._CMD_FEEDBACK_READ_STATUS)

        setpoint, last_sample, last_error, last_duty, integrator = \
            struct.unpack('<3i2q', data)

        print(setpoint, last_sample, last_error, last_duty, integrator)

    def fb_config(self, p, i, d):

        data = struct.pack('<3i', p, i, d)
        self._send_command(self._CMD_FEEDBACK_CONFIG, data)



    def _process_channel_data(self, channel_array):
    
        values = np.bitwise_and(channel_array, 0x7FFFFF).astype('int32')
        # Generate logical array of signs (+ = 0, - = 1)
        signs  = np.bitwise_and(channel_array, 1 << 23) != 0 
        
        values = np.where(signs, values - 0x800000, values)
        
        float_values = values*2.5/(0x800000*1.0)
        
        return float_values

        
p = OvenPIC()
p.fb_stop()

response = p.echo('hi\n')
print(response)
response = p.adc_read_last_conversion()
print(response)

p.adc_decimate(0)
#p.adc_start_streaming([4,5,6,7])
#p.adc_poll_stream(5)
print(p.adc_read_last_conversion())
print('set duty')
p.pwm_set_duty(0.1)
time.sleep(1)
print(p.adc_read_last_conversion())

#p.adc_poll_stream(10)
#data = p.adc_stop_streaming()
#p.pwm_set_duty(0)

#print(len(data[6]))

#np.savetxt('data.txt', np.transpose(data[4:8])) 

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


