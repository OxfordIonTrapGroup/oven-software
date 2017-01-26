
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
    
    def __init__(self, bus=1, channel=0, clockMHz=20, decimation=64):
        self._spi = SPI(bus, channel)
        
        self._spi.msh = clockMHz*1000000
        self._spi.mode = 0
        self._spi.cshigh = False
        self._spi.lsbfirst = False
        self._spi.open(bus, channel)
        
        self._setupResetPin()
        self._reset()
    
        self.setHighPowerMode()
        self.setReferenceInternal()
        self.setDecimation(decimation)
    
    def _setupResetPin(self):
        GPIO.setup(self._reset_pin, GPIO.OUT)
        GPIO.output(self._reset_pin, GPIO.HIGH)
    
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
        self._write(self._ADC_MUX_CONFIG, 0x40)
        
    def setHighPowerMode(self):
        """Puts the ADC into high power mode"""
        self._write(self._GENERAL_USER_CONFIG_1, 0x64)
        
    def setDecimation(self, decimation):
        """Set the data decimation factor between 64 and 2048"""
        if decimation < 64 or decimation > 2048:
            raise ADCException("Decimation factor out of range: " + str(decimation))
        decimation = int(decimation)
        
        self._write(self._SRC_N_MSB, (decimation & 0xFF00) >> 8)
        self._write(self._SRC_N_LSB, (decimation & 0x00FF) >> 0)
        
        
    def readErrors(self):
        """Read all the error flags"""
        
        print("GEN_ERR_REG_1: " + hex(self._read(self._GEN_ERR_REG_1)))
        print("GEN_ERR_REG_2: " + hex(self._read(self._GEN_ERR_REG_2)))
        print("STATUS_REG_1: " + hex(self._read(self._STATUS_REG_1)))
        print("STATUS_REG_2: " + hex(self._read(self._STATUS_REG_2)))
        print("STATUS_REG_3: " + hex(self._read(self._STATUS_REG_3)))
    
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
        


def selectReference():
    # use internal reference
    spi.xfer2([0x15,0x40])

def setSDMode():
    spi.xfer2([0x14,0x00]) # disable crc header

    # set general user config_3 bit 4 for S-D conversion
    spi.xfer2([0x13,0x90]) 

def setDecimation():
    spi.xfer2([0x61,0x08]) # set 1 Khz ODR
    spi.xfer2([0x62,0x00])
    
    
def readSample():
    data = spi.xfer2([0x80,0xFF,0x80,0xFF]*3)
    id = (data[0] & 0x70) >> 4
    print id, (data[4] & 0x70) >> 4, (data[8] & 0x70) >> 4,
    for x in data:
        print hex(x),
    print '.'

def readReg(reg):
    d = spi.xfer2([0x80+reg,0x00])
    for x in d:
        print hex(x),
    print '.' 

reset()

a = ADC()

a.readErrors()
print hex(a._read(0x11))
a.readErrors()

a.enableReadout(True)
while(1):
    channels, values = a.readSamples()
    print [hex(c) for c in channels]
    print [c for c in values]
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

