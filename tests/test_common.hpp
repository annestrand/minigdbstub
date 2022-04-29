#ifndef MINIGDBSTUB_TEST_COMMON_HPP
#define MINIGDBSTUB_TEST_COMMON_HPP

#include <iostream>
#include <vector>
#include "minigdbstub.h"

typedef struct {
    unsigned int softBreak  : 1;
    unsigned int hardBreak  : 1;
    unsigned int isSet      : 1;
    unsigned int isClear    : 1;
} testBreakBits;
typedef struct {
    testBreakBits bits;
    size_t addr;
} testBreak;

// Test globals
static std::vector<char> *g_getcharPktHandle, *g_putcharPktHandle;
static std::vector<unsigned char> *g_memHandle;
static int g_getcharPktIndex;

// Mock putchar
static void minigdbstubUsrPutchar(char c, void *usrData) {
    g_putcharPktHandle->push_back(c);
    return;
}

// Mock getchar
static char minigdbstubUsrGetchar(void *usrData) {
    return (*g_getcharPktHandle)[g_getcharPktIndex++];
}

// Mock read memory
static unsigned char minigdbstubUsrReadMem(size_t addr, void *usrData) {
    return (*g_memHandle)[addr];
}

// Mock write memory
static void minigdbstubUsrWriteMem(size_t addr, unsigned char data, void *usrData) {
    (*g_memHandle)[addr] = data;
    return;
}

// Mock continue
static void minigdbstubUsrContinue(void *usrData) { return; }

// Mock step
static void minigdbstubUsrStep(void *usrData) { return; }

// Mock process breakpoint
static void minigdbstubUsrProcessBreakpoint(int type, size_t addr, void *usrData) {
    testBreak *brkObj = (testBreak*)usrData;
    brkObj->addr = addr;

    if (type & MGDB_HARD_BREAKPOINT)  brkObj->bits.hardBreak = 1;
    if (type & MGDB_SOFT_BREAKPOINT)  brkObj->bits.softBreak = 1;
    if (type & MGDB_CLEAR_BREAKPOINT) brkObj->bits.isClear = 1;
    if (type & MGDB_SET_BREAKPOINT)   brkObj->bits.isSet = 1;
    return;
}

static void minigdbstubUsrKillSession(void *usrData) { return; }

#define GTEST_COUT std::cerr << "\033[0;32m[ INFO     ] \033[0;37m"
#define GTEST_FAIL_IF_ERR(x) if (x != MGDB_SUCCESS) { FAIL() << #x << " != MGDB_SUCCESS"; }

#endif // MINIGDBSTUB_TEST_COMMON_HPP