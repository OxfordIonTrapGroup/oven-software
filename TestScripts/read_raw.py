
import time

import serial

command_port="/dev/ttyO1"
data_port="/dev/ttyO4"

timeout=1
sc = serial.Serial(
    port=command_port,
    baudrate=900000,
    timeout=timeout
    )

while(1):
    print(sc.readline())


