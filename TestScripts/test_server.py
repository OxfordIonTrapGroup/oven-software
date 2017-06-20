

from artiq.protocols.pc_rpc import Client
import time



oven = Client("10.255.6.151", 5000, "atomic_oven_controller")

try:

    while(1):
        values = oven.adc_read_calibrated_sample()
        print(values[1]["temperature"])
        time.sleep(0.1)

finally:
    oven.close_rpc()


