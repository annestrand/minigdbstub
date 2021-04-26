#include <vector>
#include <iostream>
#include <string>
#include <signal.h>
#include <algorithm>

#include "test_common.hpp"
#include "minigdbstub.h"
#include "gtest/gtest.h"

TEST(minigdbstub, test_m) {
    gdbPacket mockPkt;
    mgdbProcObj mgdbObj = {0};
    initDynCharBuffer(&mockPkt.pktData, 32);

    // Create mock memory
    std::vector<unsigned char> dummyMem(128);
    dummyMem[8] = 'c';
    dummyMem[9] = 'o';
    dummyMem[10] = 'o';
    dummyMem[11] = 'l';
    g_memHandle = &dummyMem;

    // Create mock putchar buff
    std::vector<char> dummyPutchar;
    g_putcharPktHandle = &dummyPutchar;

    // Read 4 bytes starting at address 0x8
    insertDynCharBuffer(&mockPkt.pktData, 'm');
    insertDynCharBuffer(&mockPkt.pktData, '8');
    insertDynCharBuffer(&mockPkt.pktData, ',');
    insertDynCharBuffer(&mockPkt.pktData, '4');
    insertDynCharBuffer(&mockPkt.pktData, 0);

    minigdbstubReadMem(&mgdbObj, &mockPkt);
    
    // Verify putchar buffer matches the dummyMem
    for (int i=0; i<4; ++i) {
        char itoaBuff[3];
        HEX_ENCODE_ASCII(dummyMem[i+8], 3, itoaBuff);
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
    
    freeDynCharBuffer(&mockPkt.pktData);
}

TEST(minigdbstub, test_M) {
    mgdbProcObj mgdbObj = {0};
    gdbPacket mockPkt;
    initDynCharBuffer(&mockPkt.pktData, 32);

    // Create mock memory
    std::vector<unsigned char> dummyMem(128);
    g_memHandle = &dummyMem;

    // Create mock putchar handle
    std::vector<char> putcharHandle;
    g_putcharPktHandle = &putcharHandle;

    // Write '0xdeadbeef' starting at address 0x4
    insertDynCharBuffer(&mockPkt.pktData, 'M');
    insertDynCharBuffer(&mockPkt.pktData, '4');
    insertDynCharBuffer(&mockPkt.pktData, ',');
    insertDynCharBuffer(&mockPkt.pktData, '4');
    insertDynCharBuffer(&mockPkt.pktData, ':');
    insertDynCharBuffer(&mockPkt.pktData, 'd');
    insertDynCharBuffer(&mockPkt.pktData, 'e');
    insertDynCharBuffer(&mockPkt.pktData, 'a');
    insertDynCharBuffer(&mockPkt.pktData, 'd');
    insertDynCharBuffer(&mockPkt.pktData, 'b');
    insertDynCharBuffer(&mockPkt.pktData, 'e');
    insertDynCharBuffer(&mockPkt.pktData, 'e');
    insertDynCharBuffer(&mockPkt.pktData, 'f');
    insertDynCharBuffer(&mockPkt.pktData, 0);

    minigdbstubWriteMem(&mgdbObj, &mockPkt);

    const unsigned char expectedValue[] = {(unsigned char)222, (unsigned char)173, 
        (unsigned char)190, (unsigned char)239, (unsigned char)0};
    for (int i=0; i<4; ++i) {
        EXPECT_EQ(dummyMem[4+i], expectedValue[i]);
    }
}