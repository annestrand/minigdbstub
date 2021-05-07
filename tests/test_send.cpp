#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <signal.h>

#include "test_common.hpp"
#include "minigdbstub.h"
#include "gtest/gtest.h"

static void cmpCharArrays(char *arr1, char *arr2, size_t size) {
    for (size_t i=0; i<size; ++i) {
        EXPECT_EQ(arr1[i], arr2[i]) << "Chars " << arr1[i] << " and " << arr2[i] << " differ\n";
    }
}

static void buildSignalPkt(std::stringstream *s, int signal) {
    char checksum[8];
    *(s) << "+$S" << std::setw(2) << std::setfill('0') << signal;
    minigdbstubComputeChecksum(const_cast<char*>(s->str().c_str()+2), 3, checksum);
    *(s) << "#" << checksum[0] << checksum[1];
}

// --- Tests ---

TEST(minigdbstub, basic_signals) {
    // Create signal test-packets
    std::stringstream sigtrapPkt;
    buildSignalPkt(&sigtrapPkt, SIGTRAP);
    std::stringstream sigintPkt;
    buildSignalPkt(&sigintPkt, SIGINT);
    std::stringstream sigbusPkt;
    buildSignalPkt(&sigbusPkt, SIGBUS);

    // Create test putchar buffer
    std::vector<char> testBuff;
    g_putcharPktHandle = &testBuff;

    GTEST_COUT << "Testing SIGTRAP...\n";
    mgdbProcObj testObj = {0};
    testObj.signalNum = SIGTRAP;
    minigdbstubSendSignal(&testObj);
    cmpCharArrays(const_cast<char*>(sigtrapPkt.str().c_str()),
        testBuff.data(), sizeof(sigtrapPkt.str().c_str())-1);
    testBuff.clear();

    GTEST_COUT << "Testing SIGINT...\n";
    testObj.signalNum = SIGINT;
    minigdbstubSendSignal(&testObj);
    cmpCharArrays(const_cast<char*>(sigintPkt.str().c_str()),
        testBuff.data(), sizeof(sigintPkt.str().c_str())-1);
    testBuff.clear();

    GTEST_COUT << "Testing SIGBUS...\n";
    testObj.signalNum = SIGBUS;
    minigdbstubSendSignal(&testObj);
    cmpCharArrays(const_cast<char*>(sigbusPkt.str().c_str()),
        testBuff.data(), sizeof(sigbusPkt.str().c_str())-1);
    testBuff.clear();
}