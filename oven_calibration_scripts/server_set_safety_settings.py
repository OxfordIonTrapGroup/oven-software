
import time

from artiq.protocols.pc_rpc import Client
p = Client("127.0.0.1", 5000, "atomic_oven_controller")

p.settings_set_to_factory()

safety_settings = p.safety_read_channel(1)
print(safety_settings)

p.safety_set_channel(1, "duty_max", 0.6)
#p.safety_set_channel(1, "duty_max", 0.4)
p.safety_set_channel(1, "oven_temperature_max", 450)
p.safety_set_channel(1, "oven_current_max", 10)

p.settings_save()

safety_settings = p.safety_read_channel(1)
print(safety_settings)