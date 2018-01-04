#!/usr/bin/env python
# Read all of the user adjustable settings to file

import atomic_oven_controller
import argparse
import cPickle as pickle

parser = argparse.ArgumentParser()
parser.add_argument("file")
args = parser.parse_args()

p = atomic_oven_controller.OvenPICInterface(timeout=2)

settings = p.settings_read()
print(settings)

with open(args.file, "w") as f:
     f.write(pickle.dumps(settings))