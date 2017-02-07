
import time
import struct
from Adafruit_BBIO.SPI import SPI
import Adafruit_BBIO.GPIO as GPIO

GPIO.setup("P9_26", GPIO.OUT)
GPIO.output("P9_26", GPIO.HIGH)
def reset():
    GPIO.output("P9_26", GPIO.LOW)
    time.sleep(0.1)
    GPIO.output("P9_26", GPIO.HIGH)
    time.sleep(0.1)

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
    
    def __init__(self, bus=1, channel=0, clockMHz=20, decimation=2048):
        self._spi = SPI(bus, channel)
        
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
        


# def selectReference():
#     # use internal reference
#     spi.xfer2([0x15,0x40])

# def setSDMode():
#     spi.xfer2([0x14,0x00]) # disable crc header

#     # set general user config_3 bit 4 for S-D conversion
#     spi.xfer2([0x13,0x90]) 

# def setDecimation():
#     spi.xfer2([0x61,0x08]) # set 1 Khz ODR
#     spi.xfer2([0x62,0x00])
    
    
# def readSample():
#     data = spi.xfer2([0x80,0xFF,0x80,0xFF]*3)
#     id = (data[0] & 0x70) >> 4
#     print id, (data[4] & 0x70) >> 4, (data[8] & 0x70) >> 4,
#     for x in data:
#         print hex(x),
#     print '.'

# def readReg(reg):
#     d = spi.xfer2([0x80+reg,0x00])
#     for x in d:
#         print hex(x),
#     print '.' 

reset()

a = ADC()

a.readErrors()
print hex(a._read(0x11))
a.readErrors()

a.readGains()
a.readChannelErrors()

a.enableReadout(True)
tStart = time.time()
while(1):
    #a.waitForEdge()
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
        #print value*3*2.5, value2*2.5*10,value3*3*2.5
        index = channels.index(6)
        value = values[index]
        TCVoltageMV = value*2.5*1000.0/51.
        R = VOven/I

        print '%0.2f'%((-TCVoltageMV*1000.0/40.0) + 20),
        print '%0.4f'%(VOven),
        print '%0.4f'%(VOut),
        print '%0.2f'%(I),
        print '%0.2f'%(R*1000.0)
        #print channels
    except ValueError:
        pass
    #print [hex(c) for c in channels]
    #print [c for c in values]
    
    #break
    time.sleep(0.1)
 
# selectReference()
# setDecimation()
# readReg(0x5C)

# readReg(0x59)
# readReg(0x5B)
# readReg(0x5D)

# setSDMode()

# while(1):
    # readSample()

    # time.sleep(1)

