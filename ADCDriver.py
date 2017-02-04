
import time
import struct
from Adafruit_BBIO.SPI import SPI
import Adafruit_BBIO.GPIO as GPIO
import numpy as np
import struct

def crc_remainder(wordHigh, wordLow):
    '''Calculates the CRC remainder of a string of bits using a chosen polynomial. initial_filler should be '1' or '0'.'''
    len_polynomial = 9
    polynomial_bitstring = np.binary_repr(0x107,width=9)
    #print((polynomial_bitstring))  
    
    fullValue = (wordLow&0xFFFFFF) + (((wordLow>>28)&0x0F)<<24)
    fullValue += ((wordHigh&0xFFFFFF)<<28) + (((wordHigh>>28)&0x0F)<<52)
    # fullValue = (((wordHigh>>31)&0x01)) + (((wordHigh>>28)&0x07)<<1) + ((wordHigh&0xFFFFFF)<<4)
    # fullValue += (((wordLow>>31)&0x01)<<28) + (((wordLow>>28)&0x07)<<29) + ((wordLow&0xFFFFFF)<<32)
    print(hex(fullValue))
    
    # fullValue = np.bitwise_or( (wordLow&0xFFFFFF, \
                # np.left_shift(np.bitwise_and(wordHigh, 0xFFFFFF), 24) )
    input_bitstring = np.binary_repr(fullValue, width=56)
    #print(input_bitstring)
    range_len_polynomial = range(len_polynomial)
    len_input = len(input_bitstring) 
    input_padded_array = list(input_bitstring + '1'*(len_polynomial - 1))
    while '1' in input_padded_array[:len_input]:
        cur_shift = input_padded_array.index('1')
        for i in range_len_polynomial:
            #print(i, cur_shift + i)
            input_padded_array[cur_shift + i] = '0' if polynomial_bitstring[i] == input_padded_array[cur_shift + i] else '1'
    #print( ''.join(input_padded_array)[len_input:] )
    return int(''.join(input_padded_array)[len_input:], 2)

def CRC(wordHigh, wordLow):

    data = [0]*7
    data[0] = (((wordHigh >> 28) & 0x0F)<<4) + ((wordHigh >> 20) &0x0F)
    data[1] = ((wordHigh >> 12) & 0xFF)
    data[2] = ((wordHigh >> 4) & 0xFF)
    data[3] = ((wordHigh << 4) & 0xF0) + ((wordLow >> 28) & 0x0F)
    data[4] = ((wordLow >> 16) & 0xFF)
    data[5] = ((wordLow >> 8) & 0xFF)
    data[6] = ((wordLow >> 0) & 0xFF)
    
    data[0] = ~data[0]
    size = 7
    i = 0
    di = 0
    crc = 0
    while(size):
        i =  0x80
        while( i != 0 ):
            if(((crc&0x80) != 0) != ((data[di] & i) != 0)):
                crc = (crc << 1)
                crc = crc ^ 0x107
            else:
                crc = (crc << 1)
            i = i >> 1
        di += 1
        size -= 1
    return (crc&0xff)

GPIO.setup("P9_26", GPIO.OUT)

class ADCException(Exception):
    pass

class ADC:
    _reset_pin = "P9_26"
    _dataready_pin = "P9_27"
    _read_OK = 0x20
    
    _GENERAL_USER_CONFIG_1 = 0x11
    _GENERAL_USER_CONFIG_2 = 0x12
    _GENERAL_USER_CONFIG_3 = 0x13
    _DOUT_FORMAT           = 0x14
    _ADC_MUX_CONFIG        = 0x15
    _GLOBAL_MUX_CONFIG     = 0x16
    
    _GEN_ERR_REG_1    = 0x59
    _GEN_ERR_REG_1_EN = 0x5A
    _GEN_ERR_REG_2    = 0x5B
    _GEN_ERR_REG_2_EN = 0x5C
    _STATUS_REG_1     = 0x5D
    _STATUS_REG_2     = 0x5E
    _STATUS_REG_3     = 0x5F
    _SRC_N_MSB        = 0x60
    _SRC_N_LSB        = 0x61
    _SRC_IF_MSB       = 0x62
    _SRC_IF_LSB       = 0x63
    _SRC_UDPATE       = 0x64
    
    def __init__(self, bus=1, channel=0, clockMHz=20, decimation=2048, checkCRC=True):
        self._spi = SPI(bus, channel)
        
        self._checkCRC = checkCRC
        
        self._spi.msh = clockMHz*1000000
        self._spi.mode = 3
        self._spi.cshigh = False
        self._spi.lsbfirst = False
        self._spi.open(bus, channel)
        
        self._setupResetPin()
        self._setupDataReadyPin()
        self._reset()
    
        self.setHighPowerMode()
        self.setReferenceInternal()
        #self.setReferenceSupply()
        self.setDecimation(decimation)
        
        if self._checkCRC:
            self.enableCRC()
    
    def _setupResetPin(self):
        GPIO.setup(self._reset_pin, GPIO.OUT)
        GPIO.output(self._reset_pin, GPIO.HIGH)
        
    def _setupDataReadyPin(self):
        GPIO.setup(self._dataready_pin, GPIO.IN)
        #GPIO.add_event_detect(self._dataready_pin, GPIO.RISING)
    
    def waitForEdge(self):
        GPIO.wait_for_edge(self._dataready_pin, GPIO.RISING)
        
    
    def _reset(self):
        """Toggle the reset pin"""
        GPIO.output(self._reset_pin, GPIO.LOW)
        time.sleep(0.05)
        GPIO.output(self._reset_pin, GPIO.HIGH)
        time.sleep(0.05)

    
    def _read(self, address):
        """Read the value of a register"""
        data = self._spi.xfer2([address | 0x80, 0x00])
        if data[0] != self._read_OK:
            raise ADCException("ADC returned bad read header: " +str(data))
        return data[1]

    def _write(self, address, value):
        """Write a value to a register"""
        self._spi.xfer2([address, value])
        
    #########################################
        
    def disableCRC(self):
        """Disable CRC header in SPI mode"""
        self._write(self._DOUT_FORMAT, 0x00)
    def enableCRC(self):
        """Disable CRC header in SPI mode"""
        self._write(self._DOUT_FORMAT, 0x20)

        
    def setReferenceInternal(self):
        """Use the internal reference"""
        self._write(self._ADC_MUX_CONFIG, 0x58)

    def setReferenceSupply(self):
        """Use the supply as reference"""
        self._write(self._ADC_MUX_CONFIG, 0x98)
        
    def setHighPowerMode(self):
        """Puts the ADC into high power mode"""
        self._write(self._GENERAL_USER_CONFIG_1, 0x74)
        
    def setDecimation(self, decimation):
        """Set the data decimation factor between 64 and 2048"""
        if decimation < 64 or decimation > 2048:
            raise ADCException("Decimation factor out of range: " + str(decimation))
        decimation = int(decimation)
        
        self._write(self._SRC_N_MSB, (decimation & 0xFF00) >> 8)
        self._write(self._SRC_N_LSB, (decimation & 0x00FF) >> 0)

        self._write(self._SRC_UDPATE, 0x01)
        
        
    def readErrors(self):
        """Read all the error flags"""
        
        print("GEN_ERR_REG_1: " + hex(self._read(self._GEN_ERR_REG_1)))
        print("GEN_ERR_REG_2: " + hex(self._read(self._GEN_ERR_REG_2)))
        print("STATUS_REG_1: " + hex(self._read(self._STATUS_REG_1)))
        print("STATUS_REG_2: " + hex(self._read(self._STATUS_REG_2)))
        print("STATUS_REG_3: " + hex(self._read(self._STATUS_REG_3)))
    
    def readGains(self):
        """Read the channel gains"""

        for i in range(8):
            print("Channel "+str(i)+" gain: " + hex(self._read(i)))

    def readChannelErrors(self):
        """Read errors for a channel"""

        print("Channel 4/5: " + hex(self._read(0x56)))


    def enableReadout(self, enable=True):
        """Enable and disable sigma-delta readout mode"""
        if enable:
            value = 0x90
        else:
            value = 0x80
            
        self._write(self._GENERAL_USER_CONFIG_3, value)
        
    def readSamples(self, n=8):
        """Read all 8 channels"""
        
        data = self._spi.xfer2([0x80,0x00,0x80,0x00]*n)
        dataWords = np.frombuffer(bytearray(data), dtype=np.dtype(np.uint32).newbyteorder('>'), count=8)
        if self._checkCRC:
        
            for i in range(4):
                CRC_Recv =  ((dataWords[2*i] >> 24) & 0x0F) << 4
                CRC_Recv += ((dataWords[2*i + 1] >> 24) & 0x0F)
                
                CRC_Calc = CRC( dataWords[2*i], dataWords[2*i + 1]  )
                if CRC_Calc != CRC_Recv:
                    print('CRC failed: ', i)
                    print(hex(CRC_Recv), hex(CRC_Calc))

            #CRC0 = ((data[0] & 0x0F) << 4) + (data[4] & 0x0F)
            #print(hex(CRC0))

            # wordHigh = (data[0] << 24) \
                    # + (data[1] << 16) \
                    # + (data[2] << 8) \
                    # + (data[3] << 0)
                    
            # wordLow = (data[4] << 24) \
                    # + (data[5] << 16) \
                    # + (data[6] << 8) \
                    # + (data[7] << 0)

            #print(hex(wordHigh))
            #print(hex(wordLow))

            # CRC0Calc = CRC( wordHigh, wordLow  )
            #CRC0Calc = crc_remainder( wordLow, wordHigh   )
            
            
        
        channels = []
        values = []
        for i in range(n):
            channels += [(data[i*4] & 0x70) >> 4]
            value = (data[i*4 + 1] << 16) \
                    + (data[i*4 + 2] << 8) \
                    + (data[i*4 + 3] << 0)
                    
            if value & (1<<23):
                value = (value&0x7FFFFF)-0x800000
            value /= 0x800000*1.0
            values += [value]
        return channels, values
        


if __name__ == "__main__":



    a = ADC()

    a.readErrors()
    print hex(a._read(0x11))
    a.readErrors()

    a.readGains()
    a.readChannelErrors()

    a.enableReadout(True)
    tStart = time.time()
    while(1):

        channels, values = a.readSamples()
        try:
            index = channels.index(4)
            value = values[index]
            index2 = channels.index(5)
            value2 = values[index2]        
            index3 = channels.index(7)
            value3 = values[index3]
            I = value2*2.5*10.0
            VOven = -value3*3*2.5
            VOut = value*3*2.5

            index = channels.index(6)
            value = values[index]
            TCVoltageMV = value*2.5*1000.0/51.
            R = VOven/I

            print '%0.2f'%((-TCVoltageMV*1000.0/40.0) + 20),
            print '%0.4f'%(VOven),
            print '%0.4f'%(VOut),
            print '%0.2f'%(I),
            print '%0.2f'%(R*1000.0)

        except ValueError:
            pass
 
        time.sleep(0.1)
     


