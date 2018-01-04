#!/usr/bin/env python
# Load all user configurable settings from file and program to flash

import atomic_oven_controller
import argparse
import cPickle as pickle

parser = argparse.ArgumentParser()
parser.add_argument("file")
args = parser.parse_args()

with open(args.file, "r") as f:
     settings = pickle.load(f)

print(settings)

p = atomic_oven_controller.OvenPICInterface(timeout=2)

settings = p.settings_write()
p.settings_save()
