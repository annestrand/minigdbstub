# minigdbstub
A target-agnostic minimal GDB stub that interfaces using the GDB Remote Serial Protocol

## Features
- Implements the core GDB Remote Serial Protocol
    - Also implements the File-I/O Remote Protocol Extension
- Implemented as a single C/C++ header file 
    - Simply `#include "minigdbstub.h"` in your own target-specific handler
- Cross platform (Windows, macOS, Linux)