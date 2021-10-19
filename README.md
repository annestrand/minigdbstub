# minigdbstub
A target-agnostic minimal GDB stub that interfaces using the GDB Remote Serial Protocol.

## Features
- Implements the core GDB Remote Serial Protocol
- Implemented as a single C/C++ header file
    - `#include "minigdbstub.h"` in your own target-specific handler
- Cross platform (Windows, macOS, Linux)

## Usage
The stub has the following user (i.e. `Usr`) functions that the user has to define:
```c
// User functions
static void minigdbstubUsrPutchar(char data, void *usrData);
static char minigdbstubUsrGetchar(void *usrData);
static void minigdbstubUsrWriteMem(size_t addr, unsigned char data, void *usrData);
static unsigned char minigdbstubUsrReadMem(size_t addr, void *usrData);
static void minigdbstubUsrContinue(void *usrData);
static void minigdbstubUsrStep(void *usrData);
static void minigdbstubUsrProcessBreakpoint(int type, size_t addr, void *usrData);
static void minigdbstubUsrKillSession(void *usrData);
```
The partial code snippet below is example code of how `minigdbstub` can be used:
```c
#include <stdio.h>
#include "minigdbstub.h"

#define REG_COUNT 32

typedef struct {
    int regfile[32];
    // ...
} myCustomData;

void gdbserverCall(myCustomData *myCustomData) {
    // Update regs
    int regs[REG_COUNT];
    for (int i=0; i<REG_COUNT; ++i) {
        regs[i] = myCustomData->regFile[i];
    }

    // Create and write values to minigdbstub process call object
    mgdbProcObj mgdbObj = {0};
    mgdbObj.regs = (char*)regs;
    mgdbObj.regsSize = sizeof(regs);
    mgdbObj.regsCount = REG_COUNT;
    mgdbObj.opts.o_signalOnEntry = 1;
    mgdbObj.opts.o_enableLogging = 0;
    mgdbObj.usrData = (void*)myCustomData;

    // Call into minigdbstub
    minigdbstubProcess(&mgdbObj);

    return;
}

// User-defined minigdbstub handlers
static void minigdbstubUsrWriteMem(size_t addr, unsigned char data, void *usrData) {
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}

static unsigned char minigdbstubUsrReadMem(size_t addr, void *usrData) {
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}

static void minigdbstubUsrContinue(void *usrData) {
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}

static void minigdbstubUsrStep(void *usrData) {
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}

static char minigdbstubUsrGetchar(void *usrData)
{
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}

static void minigdbstubUsrPutchar(char data, void *usrData)
{
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}

static void minigdbstubUsrProcessBreakpoint(int type, size_t addr, void *usrData) {
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}

static void minigdbstubUsrKillSession(void *usrData) {
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}

// ...
```

The `usrData` serves as opaque data that is forwarded to the `Usr` type functions - this allows users of
minigdbstub to not have to use globals.

## Building unit tests
[GoogleTest](https://github.com/google/googletest) is used as the unit testing framework. So you will
need to have GoogleTest installed on your system for CMake to pick-up as a package.

To build the unit tests:
```
cmake . -Bbuild
cmake --build build
```

