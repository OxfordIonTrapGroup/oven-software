

import numpy as np


def crc_remainder(wordHigh, wordLow):
	'''Calculates the CRC remainder of a string of bits using a chosen polynomial. initial_filler should be '1' or '0'.'''
	len_polynomial = 9
	polynomial_bitstring = np.binary_repr(0x107,width=9)
	#print((polynomial_bitstring))	
	fullValue = np.bitwise_or( wordLow&0xFFFFFF, \
				np.left_shift(np.bitwise_and(wordHigh, 0xFFFFFF), 24) )
	input_bitstring = np.binary_repr(fullValue, width=48)
	#print(input_bitstring)
	range_len_polynomial = range(len_polynomial)
	len_input = len(input_bitstring) 
	input_padded_array = list(input_bitstring + '0'*(len_polynomial - 1))
	while '1' in input_padded_array[:len_input]:
		cur_shift = input_padded_array.index('1')
		for i in range_len_polynomial:
			#print(i, cur_shift + i)
			input_padded_array[cur_shift + i] = '0' if polynomial_bitstring[i] == input_padded_array[cur_shift + i] else '1'
	#print( ''.join(input_padded_array)[len_input:] )
	return int(''.join(input_padded_array)[len_input:], 2)

def crc_remainder2(input_bitstring, polynomial_bitstring, initial_filler):
	'''Calculates the CRC remainder of a string of bits using a chosen polynomial. initial_filler should be '1' or '0'.'''
	len_polynomial = len(polynomial_bitstring)
	range_len_polynomial = range(len_polynomial)
	len_input = len(input_bitstring)
	input_padded_array = list(input_bitstring + initial_filler*(len_polynomial - 1))
	while '1' in input_padded_array[:len_input]:
		cur_shift = input_padded_array.index('1')
		for i in range_len_polynomial:
			#print(i, cur_shift + i)
			input_padded_array[cur_shift + i] = '0' if polynomial_bitstring[i] == input_padded_array[cur_shift + i] else '1'
	return ''.join(input_padded_array)[len_input:]


def CRC( wordHigh, wordLow ):
	'''Calculates x^8+x^2+x+1 CRC of two 24bit samples'''

	polynomial = np.left_shift(0x107, 47)

	fullData = wordLow & 0xFFFFFF
	fullData = np.bitwise_or( fullData, np.left_shift(np.bitwise_and(wordHigh, 0xFFFFFF), 24) )
	fullData = np.left_shift( fullData, 8 )
	print(hex(fullData))		
	print(hex(polynomial))	

	for i in range(48):
		fullData = np.bitwise_and(np.bitwise_xor( fullData, polynomial ),)
		
		polynomial = np.right_shift( polynomial, 1 )	
		
		print(i)
		print(hex(fullData))		
		print(hex(polynomial))	
		
	result = np.bitwise_and(fullData, 0xFF)
	print(np.binary_repr(result))
 
def crc16(data, bits=8):
    crc = 0xFFFF
    for op, code in zip(data[0::2], data[1::2]):
        crc = crc ^ int(op+code, 16)
        for bit in range(0, bits):
            if (crc&0x0001)  == 0x0001:
                crc = ((crc >> 1) ^ 0xA001)
            else:
                crc = crc >> 1
    return typecasting(crc)

def typecasting(crc):
    msb = hex(crc >> 8)
    lsb = hex(crc & 0x00FF)
    return lsb + msb
    
    
def myCRC(data, size):
    data[0] = ~data[0]
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

    
def CRC(wordHigh, wordLow):

    fullValue = (wordLow&0xFFFFFF) + (((wordLow>>28)&0x0F)<<24)
    fullValue += ((wordHigh&0xFFFFFF)<<28) + (((wordHigh>>28)&0x0F)<<52)

    
    
    data = [0]*7
    data[0] = (((wordHigh >> 28) & 0x0F)<<4) + ((wordHigh >> 20) &0x0F)
    data[1] = ((wordHigh >> 12) & 0xFF)
    data[2] = ((wordHigh >> 4) & 0xFF)
    data[3] = ((wordHigh << 4) & 0xF0) + ((wordLow >> 28) & 0x0F)
    data[4] = ((wordLow >> 16) & 0xFF)
    data[5] = ((wordLow >> 8) & 0xFF)
    data[6] = ((wordLow >> 0) & 0xFF)

    for c in data:
        print(hex(c)),
    print ''
    
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
    
print( hex(CRC( 0x8c001881L, 0x93000015 )))

#print( crc_remainder2('1111100101001110', '100000111', '0') )
#print( crc_remainder(0x00,0xF94E) )
#print(hex(myCRC([0x80,0x01,0x88,0x19,0x00,0x00,0x15],7)))
