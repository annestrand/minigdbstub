#include <vector>
#include <iostream>
#include <string>
#include <signal.h>

#include "test_common.hpp"
#include "minigdbstub.h"
#include "gtest/gtest.h"

// --- Tests ---

TEST(minigdbstub, test_set_soft_breakpoint) {
    // Create mock test packet
    const char *packet = "$Z0,d8,4#b2";

    // Create mock putchar buff
    std::vector<char> dummyPutchar;
    g_putcharPktHandle = &dummyPutchar;

    testBreak breakObj  = {{0}};
    mgdbProcObj procObj = {0};
    procObj.usrData     = &breakObj;
    gdbPacket gdbPkt;
    GTEST_FAIL_IF_ERR(initDynCharBuffer(&gdbPkt.pktData, 32));
    gdbPkt.commandType = 'Z';
    gdbPkt.checksum[0] = 'b';
    gdbPkt.checksum[1] = '2';
    gdbPkt.checksum[2] = 0;
    for (size_t i=0; i<strlen(packet); ++i) {
        GTEST_FAIL_IF_ERR(insertDynCharBuffer(&gdbPkt.pktData, packet[i]));
    }
    minigdbstubProcessBreakpoint(&procObj, &gdbPkt, MGDB_SET_BREAKPOINT);
    GTEST_FAIL_IF_ERR(procObj.err);

    testBreak *brkObj = (testBreak*)procObj.usrData;
    EXPECT_EQ(brkObj->addr, 0xd8U);
    EXPECT_EQ(brkObj->bits.softBreak, 1U);
    EXPECT_EQ(brkObj->bits.isSet, 1U);
    EXPECT_EQ(brkObj->bits.hardBreak, 0U);
    EXPECT_EQ(brkObj->bits.isClear, 0U);
    freeDynCharBuffer(&gdbPkt.pktData);
}