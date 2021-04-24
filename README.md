# minigdbstub
A target-agnostic minimal GDB stub that interfaces using the GDB Remote Serial Protocol.

## Features
- Implements the core GDB Remote Serial Protocol
    - Also implements the File-I/O Remote Protocol Extension
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