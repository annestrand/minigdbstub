#include <vector>
#include <iostream>
#include <string>
#include <signal.h>

#include "minigdbstub.h"
#include "gtest/gtest.h"

#define GTEST_COUT std::cerr << "\033[0;32m[ INFO     ] \033[0;37m"

// Test globals
std::vector<char> g_recvPacket;

// Mock putchar
static void minigdbstubUsrPutchar(char c) {
    GTEST_COUT << "Recieved char: " << c << std::endl;
    g_recvPacket.push_back(c);
}

// Mock getchar
static char minigdbstubUsrGetchar(void) {
    return ' ';
}

static void cmpCharArrays(char *arr1, char *arr2, size_t size) {
    for (int i=0; i<size; ++i) {
        EXPECT_EQ(arr1[i], arr2[i]) << "Chars " << arr1[i] << " and " << arr2[i] << " differ\n";
    }
}

// --- Tests ---

TEST(minigdbstub, basic_signals) {
    char goldSigtrap[] = "+$S5#88";
    char goldSigint[]  = "+$S2#85";
    char goldSigbus[]  = "+$S7#8a";
    g_recvPacket.clear();

    // Test some signals
    GTEST_COUT << "Testing SIGTRAP...\n";
    minigdbstubSendSignal(SIGTRAP);
    cmpCharArrays(goldSigtrap, g_recvPacket.data(), sizeof(goldSigtrap)-1);
    g_recvPacket.clear();

    GTEST_COUT << "Testing SIGINT...\n";
    minigdbstubSendSignal(SIGINT);
    cmpCharArrays(goldSigint, g_recvPacket.data(), sizeof(goldSigint)-1);
    g_recvPacket.clear();

    GTEST_COUT << "Testing SIGBUS...\n";
    minigdbstubSendSignal(SIGBUS);
    cmpCharArrays(goldSigbus, g_recvPacket.data(), sizeof(goldSigbus)-1);
    g_recvPacket.clear();
}