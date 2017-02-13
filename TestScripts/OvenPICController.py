
import serial
import time
import numpy as np
import matplotlib 
matplotlib.use('Agg')
import matplotlib.pyplot as plt

class OvenPIC:
    _INS_MAGIC_START = 0xA5
    _INS_MAGIC_END   = 0x5A
    
    _CMD_ECHO = 0x00
    _CMD_READ_LAST_CONVERSION = 0x01
    _CMD_STREAM_ADC  = 0x02

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
    
    def stream_channels(self, channels, duration, block_size = 100):
        
        
        channel_byte = 0
        for channel in channels:
            channel_byte += 1 << channel
        
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
        #print(array_data[:10])
        
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
            #print(len(channel_array))
            
            channel_values.append(self._process_channel_data(channel_array))
            
        
        
        #print(channel_bytes)
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

duration = 10

channel_values = p.stream_channels([6], duration)

data = ((-channel_values[0]*(1000.0/40.0)*(1000.0/51.)) + 20)

t = np.linspace(0,duration,len(data))


plt.plot(t, data, '.')
#plt.ylim(19.8)
plt.savefig('pp.pdf')


# def sendPacket( command, payload ):
    # data = [INS_MAGIC_START]
    # data += [command]
    # data += [len(payload)]
    # data += [0]
    # data += [INS_MAGIC_END]
    # data += payload

   # s.write(data)


# s = serial.Serial(port='/dev/ttyO5', baudrate=250000, timeout=1)

# #sendPacket( CMD_ECHO, [1,2,3,4] )
# sendPacket( 2, [1] )

# d = s.read(200)
# print(len(d))
# print(d)
# for c in d:
    # print(hex(ord(c)))

# sendPacket( 2, [0] )
