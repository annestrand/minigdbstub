# minigdbstub
A target-agnostic minimal GDB stub that interfaces using the GDB Remote Serial Protocol.

## Features
- Implements the core GDB Remote Serial Protocol
- Implemented as a single C/C++ header file 
    - Simply `#include "minigdbstub.h"` in your own target-specific handler
- Cross platform (Windows, macOS, Linux)

## Unit Testing Dependencies
This repo uses the Conan C/C++ Package Manager to fetch any needed dependencies. A minimum starting
prerequisite is to have [Python3](https://www.python.org/downloads/) installed on your system.

To install Conan (if you don't have it installed already), use the Python3 package manager command below:

    $ pip3 install conan

## Building

    $ conan install -if build
    $ cmake . -Bbuild
    $ cmake --build build

## Running tests
There is a test runner Python script **tr.py** located in the project root:

    $ python3 ./tr.py [optional_test_binary_name]

If no test binary name is given, the test runner script will run through all the tests.