
import serial
import time
import numpy as np
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
    _CMD_FEEDBACK_SET_LIMITS        = 0x35

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

    def fb_set_limits(self, limit_i, limit_t):
        data = struct.pack('<ii', int(limit_i), int(limit_t))
        self._send_command(self._CMD_FEEDBACK_SET_LIMITS, data) 

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

# def TC(T):
#     return -(T - 20)*(51./1000.)*(40.0/1000.)*0x800000/2.5

# def IC(I):
#     return (I/10.0)*0x800000/2.5

# p = OvenPIC()
# p.fb_stop()

# def test_streaming_fixed():

#     p.adc_decimate(0)
#     p.adc_start_streaming([4,5,6,7])
#     p.adc_poll_stream(5)

#     p.pwm_set_duty(0.1)

#     p.adc_poll_stream(10)
#     p.pwm_set_duty(0)
#     data = p.adc_stop_streaming()

#     print(len(data[6]))

#     np.savetxt('data.txt', np.transpose(data[4:8])) 

#     dd

# def test_fb():
#     p.fb_config( -10, -2, 0 )
#     p.fb_set_setpoint( TC(20) )
#     p.fb_set_limits(IC(10), TC(200))
#     p.fb_start()

#     p.adc_decimate(0)
#     p.adc_start_streaming([4,5,6,7])
#     p.adc_poll_stream(2)

#     p.fb_set_setpoint( TC(300) )
#     p.adc_poll_stream(20)
#     p.fb_set_setpoint( TC(20) )
#     p.adc_poll_stream(10)

#     p.fb_stop()
#     data = p.adc_stop_streaming()

#     print(len(data[6]))
#     np.savetxt('data.txt', np.transpose(data[4:8])) 

#     dd

# test_fb()



