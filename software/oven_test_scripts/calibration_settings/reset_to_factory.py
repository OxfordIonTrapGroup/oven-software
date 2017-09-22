
import time
import atomic_oven_controller

p = atomic_oven_controller.OvenPICInterface(timeout=2)

p.settings_set_to_factory()

p.settings_save()