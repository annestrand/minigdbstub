#include <vector>
#include <iostream>
#include <string>
#include <signal.h>

#include "test_common.hpp"
#include "minigdbstub.h"
#include "gtest/gtest.h"

static void cmpCharArrays(char *arr1, char *arr2, size_t size) {
    for (size_t i=0; i<size; ++i) {
        EXPECT_EQ(arr1[i], arr2[i]) << "Chars " << arr1[i] << " and " << arr2[i] << " differ\n";
    }
}

// --- Tests ---

TEST(minigdbstub, basic_signals) {
    char goldSigtrap[] = "+$S5#88";
    char goldSigint[]  = "+$S2#85";
    char goldSigbus[]  = "+$S7#8a";

    // Create putchar buffer
    std::vector<char> testBuff;
    g_putcharPktHandle = &testBuff;

    // Test some signals
    GTEST_COUT << "Testing SIGTRAP...\n";
    mgdbProcObj testObj;
    testObj.signalNum = SIGTRAP;
    minigdbstubSendSignal(&testObj);
    cmpCharArrays(goldSigtrap, testBuff.data(), sizeof(goldSigtrap)-1);
    testBuff.clear();

    GTEST_COUT << "Testing SIGINT...\n";
    testObj.signalNum = SIGINT;
    minigdbstubSendSignal(&testObj);
    cmpCharArrays(goldSigint, testBuff.data(), sizeof(goldSigint)-1);
    testBuff.clear();

    GTEST_COUT << "Testing SIGBUS...\n";
    testObj.signalNum = SIGBUS;
    minigdbstubSendSignal(&testObj);
    cmpCharArrays(goldSigbus, testBuff.data(), sizeof(goldSigbus)-1);
    testBuff.clear();
}