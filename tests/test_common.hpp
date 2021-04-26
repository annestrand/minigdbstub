#ifndef MINIGDBSTUB_TEST_COMMON_HPP
#define MINIGDBSTUB_TEST_COMMON_HPP

#include <iostream>
#include <vector>

// Test globals
std::vector<char> *g_getcharPktHandle, *g_putcharPktHandle;
std::vector<unsigned char> *g_memHandle;
int g_getcharPktIndex;

// Mock putchar
static void minigdbstubUsrPutchar(char c) {
    g_putcharPktHandle->push_back(c);
}

// Mock getchar
static char minigdbstubUsrGetchar(void) {
    return (*g_getcharPktHandle)[g_getcharPktIndex++];
}

// Mock read memory
static unsigned char minigdbstubUsrReadMem(size_t addr, void *usrData) {
    return (*g_memHandle)[addr];
}

// Mock write memory
static void minigdbstubUsrWriteMem(size_t addr, unsigned char data, void *usrData) {
    (*g_memHandle)[addr] = data;
}

#define GTEST_COUT std::cerr << "\033[0;32m[ INFO     ] \033[0;37m"

#endif // MINIGDBSTUB_TEST_COMMON_HPP