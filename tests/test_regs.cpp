#include <signal.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "minigdbstub.h"
#include "test_common.hpp"

TEST(minigdbstub, test_g)
{
    // Create dummy regfile
    int regs[8]         = {2, 4, 55, 6, 12, 23, 81, 1};
    size_t regSize      = sizeof(regs);
    mgdbProcObj mgdbObj = {0};
    mgdbObj.regs        = (char *)regs;
    mgdbObj.regsSize    = regSize;
    mgdbObj.regsCount   = 8;
    mgdbObj.signalNum   = 5;

    std::vector<char> testVec;
    g_putcharPktHandle = &testVec;
    g_putcharPktHandle->clear();
    minigdbstubSendRegs(&mgdbObj);
    GTEST_FAIL_IF_ERR(mgdbObj.err);

    for (size_t i = 0; i < regSize; ++i)
    {
        char itoaBuff[3];
        MGDB_HEX_ENCODE_ASCII(mgdbObj.regs[i], 3, itoaBuff);
        // Swap if single digit
        if (itoaBuff[1] == 0)
        {
            itoaBuff[1] = itoaBuff[0];
            itoaBuff[0] = '0';
        }
        EXPECT_EQ(itoaBuff[0], (*g_putcharPktHandle)[(i * 2) + 1]);
        EXPECT_EQ(itoaBuff[1], (*g_putcharPktHandle)[(i * 2) + 2]);
    }
}

TEST(minigdbstub, test_G)
{
    char charRegs[(2 * 8 * sizeof(int)) + 1] = {
        '0', 'b', '0', '0', '0', '0', '0', '0',  // regs[0] = 11
        '0', '4', '0', '0', '0', '0', '0', '0',  // regs[1] = 4
        '0', '5', '0', '0', '0', '0', '0', '0',  // regs[2] = 5
        '0', '6', '0', '0', '0', '0', '0', '0',  // regs[3] = 6
        '3', '7', '0', '0', '0', '0', '0', '0',  // regs[4] = 55
        '2', '2', '0', '0', '0', '0', '0', '0',  // regs[5] = 34
        '1', '7', '0', '0', '0', '0', '0', '0',  // regs[6] = 23
        '1', '0', '0', '0', '0', '0', '0', '0',  // regs[7] = 16
        0};
    int expectedResults[8] = {11, 4, 5, 6, 55, 34, 23, 16};
    int regs2[8]           = {1, 1, 1, 1, 1, 1, 1, 1};

    gdbPacket recvPkt;
    recvPkt.pktData.buffer = charRegs;
    recvPkt.pktData.size   = sizeof(charRegs);

    mgdbProcObj procObj = {0};
    procObj.regs        = (char *)regs2;

    minigdbstubWriteRegs(&procObj, &recvPkt);
    GTEST_FAIL_IF_ERR(procObj.err);
    for (size_t i = 0; i < sizeof(expectedResults) / sizeof(int); i++)
    {
        EXPECT_EQ(expectedResults[i], regs2[i]);
    }
}

TEST(minigdbstub, test_p)
{
    gdbPacket mockPkt;
    mgdbProcObj mgdbObj = {0};
    int regs[8]         = {1, 1, 1, 1, 1, 50, 1, 1};
    mgdbObj.regs        = (char *)regs;
    mgdbObj.regsSize    = sizeof(regs);
    mgdbObj.regsCount   = 8;
    GTEST_FAIL_IF_ERR(initDynCharBuffer(&mockPkt.pktData, 32));

    // Read register at index 6
    GTEST_FAIL_IF_ERR(insertDynCharBuffer(&mockPkt.pktData, '$'));
    GTEST_FAIL_IF_ERR(insertDynCharBuffer(&mockPkt.pktData, 'p'));
    GTEST_FAIL_IF_ERR(insertDynCharBuffer(&mockPkt.pktData, '5'));
    GTEST_FAIL_IF_ERR(insertDynCharBuffer(&mockPkt.pktData, '#'));
    GTEST_FAIL_IF_ERR(insertDynCharBuffer(&mockPkt.pktData, 'a'));
    GTEST_FAIL_IF_ERR(insertDynCharBuffer(&mockPkt.pktData, '5'));
    GTEST_FAIL_IF_ERR(insertDynCharBuffer(&mockPkt.pktData, 0));

    std::vector<char> testVec;
    g_putcharPktHandle = &testVec;
    g_putcharPktHandle->clear();

    minigdbstubSendReg(&mgdbObj, &mockPkt);
    GTEST_FAIL_IF_ERR(mgdbObj.err);
    int actualResult = 0;
    int rotL         = 1;
    std::rotate(testVec.begin(), testVec.begin() + rotL, testVec.end());
    auto it      = std::find(testVec.begin(), testVec.end(), '#');
    *it          = 0;
    actualResult = strtol(testVec.data(), NULL, 16);
    int byte0    = actualResult & 0xff;
    int byte1    = (actualResult & 0xff00) >> 8;
    int byte2    = (actualResult & 0xff0000) >> 8 * 2;
    int byte3    = (actualResult & 0xff000000) >> 8 * 3;
    actualResult = (byte0 << 8 * 3) | (byte1 << 8 * 2) | (byte2 << 8 * 1) | byte3;
    EXPECT_EQ(regs[5], actualResult);
    freeDynCharBuffer(&mockPkt.pktData);
}

TEST(minigdbstub, test_P)
{
    gdbPacket mockPkt;
    mgdbProcObj mgdbObj = {0};
    int regs[8]         = {1, 1, 1, 1, 1, 1, 1, 1};
    mgdbObj.regs        = (char *)regs;
    mgdbObj.regsSize    = sizeof(regs);
    mgdbObj.regsCount   = 8;
    GTEST_FAIL_IF_ERR(initDynCharBuffer(&mockPkt.pktData, 32));

    // Write '23' at register index 3
    GTEST_FAIL_IF_ERR(insertDynCharBuffer(&mockPkt.pktData, 'P'));
    GTEST_FAIL_IF_ERR(insertDynCharBuffer(&mockPkt.pktData, '3'));
    GTEST_FAIL_IF_ERR(insertDynCharBuffer(&mockPkt.pktData, '='));
    GTEST_FAIL_IF_ERR(insertDynCharBuffer(&mockPkt.pktData, '1'));
    GTEST_FAIL_IF_ERR(insertDynCharBuffer(&mockPkt.pktData, '7'));
    GTEST_FAIL_IF_ERR(insertDynCharBuffer(&mockPkt.pktData, 0));

    minigdbstubWriteReg(&mgdbObj, &mockPkt);
    GTEST_FAIL_IF_ERR(mgdbObj.err);
    EXPECT_EQ(regs[3], 23);
    freeDynCharBuffer(&mockPkt.pktData);
}