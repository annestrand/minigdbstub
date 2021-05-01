#!/usr/bin/python3

# Basic test-runner script

import os
import sys
import subprocess
import argparse

# Parse test args
parser = argparse.ArgumentParser(description="Test arg parser.")
parser.add_argument("-f", "--test_filter", help="Provide optional test filter to gtest")
parser.add_argument("-l", "--list", dest="list", action="store_true", help="List available tests to run")
args = parser.parse_args()

testBinary = os.path.join(os.path.abspath(os.path.dirname(sys.argv[0])), "build", "bin", "minigdbstub_tests")

if args.list is not False:
    subprocess.run([testBinary, "--gtest_list_tests"])
    exit(0)

# Build test string
testStr = ""
if args.test_filter is not None:
    testStr += f"--gtest_filter=minigdbstub.{args.test_filter}"

# Run test(s)
subprocess.run([testBinary, testStr])