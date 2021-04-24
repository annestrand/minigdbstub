#!/usr/bin/python3

# Basic test-runner script

import os
import sys
import subprocess

# Grab all test bins
testFolder = os.path.join(os.path.abspath(os.path.dirname(sys.argv[0])), "build", "bin")

# If no arg given, run all tests - otherwise run the test-name given
testBins = list(os.listdir(testFolder))
if len(sys.argv) == 1:
    for test in testBins:
        subprocess.run([os.path.join(testFolder, test)])
else:
    subprocess.run([os.path.join(testFolder, sys.argv[1])])