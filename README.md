# minigdbstub
A target-agnostic minimal GDB stub that interfaces using the GDB Remote Serial Protocol.

## Features
- Implements the core GDB Remote Serial Protocol
- Implemented as a single C/C++ header file 
    - `#include "minigdbstub.h"` in your own target-specific handler
- Cross platform (Windows, macOS, Linux)

## Building unit tests
[GoogleTest](https://github.com/google/googletest) is used as the unit testing framework. There are
a couple of different ways to build the tests:

If GoogleTest libs are already installed in a default system location

    $ cmake . -Bbuild
    $ cmake --build build

If GoogleTest libs are already installed in a non-default system location

    $ cmake -DGTEST_BASEDIR=<path_to_gtest_libs> . -Bbuild
    $ cmake --build build

## Conan
If you don't have the gtest libs already installed, [Conan](https://docs.conan.io/en/latest/installation.html) can be
used to pull gtest in, then build with CMake.

Conan is cross-platform and implemented in Python - to install Conan:

    $ pip install conan

Use Conan to pull the gtest package and build with CMake

    $ conan install -if build conanfile.txt
    $ cmake . -Bbuild
    $ cmake --build build
