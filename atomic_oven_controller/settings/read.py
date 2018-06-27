#!/usr/bin/env python
# Read all of the user adjustable settings to file

import atomic_oven_controller.interface
import argparse
import artiq.protocols.pyon as pyon

parser = argparse.ArgumentParser(
    description="Read all of the user adjustable settings to file")
parser.add_argument("file")
args = parser.parse_args()

p = atomic_oven_controller.interface.Interface(timeout=2)

settings = p.settings_read()

pyon.store_file(args.file, settings)
