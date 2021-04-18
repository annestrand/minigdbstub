# minigdbstub
A target-agnostic minimal GDB stub that interfaces using the GDB Remote Serial Protocol - written in C++.

## Features
- Implements the core GDB Remote Serial Protocol
    - Also implements the File-I/O Remote Protocol Extension
- C/C++ friendly API for user to implement their own target-specific handlers
- Cross platform (Windows, macOS, Linux)

## Build dependencies
- CMake (v3.10 or higher)
- Some compatible CMake Generator (e.g. GNU Make, Visual Studio, Ninja)
- C++11 compiler or newer

## Building 
    $ cmake . -Bbuild
    $ cmake --build ./build