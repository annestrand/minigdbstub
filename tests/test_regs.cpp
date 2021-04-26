#include <vector>
#include <iostream>
#include <string>
#include <signal.h>
#include <algorithm>

#include "test_common.hpp"
#include "minigdbstub.h"
#include "gtest/gtest.h"

TEST(minigdbstub, test_g) {
    // Create dummy regfile
    int regs[8] = {2,4,55,6,12,23,81,1};
    size_t regSize = sizeof(regs);
    mgdbProcObj mgdbObj = {0};
    mgdbObj.regs = (char*)regs;
    mgdbObj.regsSize = regSize;
    mgdbObj.regsCount = 8;
    mgdbObj.signalNum = 5;

    std::vector<char> testVec;
    g_putcharPktHandle = &testVec;
    g_putcharPktHandle->clear();
    minigdbstubSendRegs(mgdbObj.regs, mgdbObj.regsSize);

    for (size_t i=0; i<regSize; ++i) {
        char itoaBuff[3];
        HEX_ENCODE_ASCII(mgdbObj.regs[i], 3, itoaBuff);
        for (int j=0; j<2; ++j) {
            itoaBuff[j] = (itoaBuff[j] == 0) ? ('0') : (itoaBuff[j]);
        }
        if (itoaBuff[1] == '0') {
            EXPECT_EQ(itoaBuff[1], (*g_putcharPktHandle)[i*2]);
            EXPECT_EQ(itoaBuff[0], (*g_putcharPktHandle)[(i*2)+1]);
        }
        else {
            EXPECT_EQ(itoaBuff[0], (*g_putcharPktHandle)[i*2]);
            EXPECT_EQ(itoaBuff[1], (*g_putcharPktHandle)[(i*2)+1]);
        }
    }
}

TEST(minigdbstub, test_G) {
    char charRegs[(2*8*sizeof(int))+1] = {
        '0','b', '0','0', '0','0', '0','0', // regs[0] = 11
        '0','4', '0','0', '0','0', '0','0', // regs[1] = 4
        '0','5', '0','0', '0','0', '0','0', // regs[2] = 5
        '0','6', '0','0', '0','0', '0','0', // regs[3] = 6
        '3','7', '0','0', '0','0', '0','0', // regs[4] = 55
        '2','2', '0','0', '0','0', '0','0', // regs[5] = 34
        '1','7', '0','0', '0','0', '0','0', // regs[6] = 23
        '1','0', '0','0', '0','0', '0','0', // regs[7] = 16
        0
    };
    int expectedResults[8] = {11,4,5,6,55,34,23,16};
    int regs2[8] = {1,1,1,1,1,1,1,1};

    minigdbstubWriteRegs(charRegs, (sizeof(charRegs))-1, (char*)regs2);
    for (size_t i=0; i<sizeof(expectedResults)/sizeof(int); i++) {
        EXPECT_EQ(expectedResults[i], regs2[i]);
    }
}

TEST(minigdbstub, test_p) {
    gdbPacket mockPkt;
    mgdbProcObj mgdbObj;
    int regs[8] = {1,1,1,1,1,50,1,1};
    mgdbObj.regs = (char*)regs;
    mgdbObj.regsSize = sizeof(regs);
    mgdbObj.regsCount = 8;
    initDynCharBuffer(&mockPkt.pktData, 32);

    // Read register at index 6
    insertDynCharBuffer(&mockPkt.pktData, 'p');
    insertDynCharBuffer(&mockPkt.pktData, '5');
    insertDynCharBuffer(&mockPkt.pktData, 0);

    std::vector<char> testVec;
    g_putcharPktHandle = &testVec;
    g_putcharPktHandle->clear();

    minigdbstubSendReg(&mgdbObj, &mockPkt);
    int actualResult = 0;
    int rotL = 2;
    std::rotate(testVec.begin(), testVec.begin()+rotL, testVec.end());
    testVec.push_back(0);
    actualResult = strtol(testVec.data(), NULL, 16);
    EXPECT_EQ(regs[5], actualResult);
    
    freeDynCharBuffer(&mockPkt.pktData);
}

TEST(minigdbstub, test_P) {
    gdbPacket mockPkt;
    mgdbProcObj mgdbObj;
    int regs[8] = {1,1,1,1,1,1,1,1};
    mgdbObj.regs = (char*)regs;
    mgdbObj.regsSize = sizeof(regs);
    mgdbObj.regsCount = 8;
    initDynCharBuffer(&mockPkt.pktData, 32);

    // Write '23' at register index 3
    insertDynCharBuffer(&mockPkt.pktData, 'P');
    insertDynCharBuffer(&mockPkt.pktData, '3');
    insertDynCharBuffer(&mockPkt.pktData, '=');
    insertDynCharBuffer(&mockPkt.pktData, '1');
    insertDynCharBuffer(&mockPkt.pktData, '7');
    insertDynCharBuffer(&mockPkt.pktData, 0);

    minigdbstubWriteReg(&mgdbObj, &mockPkt);
    EXPECT_EQ(regs[3], 23);
    
    freeDynCharBuffer(&mockPkt.pktData);
}