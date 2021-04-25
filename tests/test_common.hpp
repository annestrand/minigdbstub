#ifndef MINIGDBSTUB_TEST_COMMON_HPP
#define MINIGDBSTUB_TEST_COMMON_HPP

#include <iostream>
#include <vector>

// Test globals
std::vector<char> *g_getcharPktHandle, *g_putcharPktHandle;
int g_getcharPktIndex;

// Mock putchar
static void minigdbstubUsrPutchar(char c) {
    g_putcharPktHandle->push_back(c);
}

// Mock getchar
static char minigdbstubUsrGetchar(void) {
    return (*g_getcharPktHandle)[g_getcharPktIndex];
}

#define GTEST_COUT std::cerr << "\033[0;32m[ INFO     ] \033[0;37m"

#endif // MINIGDBSTUB_TEST_COMMON_HPP