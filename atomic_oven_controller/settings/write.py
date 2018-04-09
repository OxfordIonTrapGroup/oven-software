#!/usr/bin/env python
# Load all user configurable settings from file and program to flash

import atomic_oven_controller
import argparse
import artiq.protocols.pyon as pyon

parser = argparse.ArgumentParser()
parser.add_argument("file")
args = parser.parse_args()

settings = pyon.load_file(args.file)

p = atomic_oven_controller.Interface(timeout=2)

p.settings_write(settings)
p.settings_save()
