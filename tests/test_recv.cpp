#include <vector>
#include <iostream>
#include <string>
#include <signal.h>

#include "minigdbstub.h"
#include "gtest/gtest.h"

#define GTEST_COUT std::cerr << "\033[0;32m[ INFO     ] \033[0;37m"

// Test globals
std::vector<const char*> g_mockPackets = {
    "+$g#67",
    "+$G#47",
    "+$c#63",
    "+$Ga700467f#46"
};
std::vector<char> g_mockPutcharBuf; 
unsigned int g_packetChar;
const char *g_mockPacket;

// Mock getchar
static char minigdbstubUsrGetchar(void) {
    return g_mockPacket[g_packetChar++];
}

// Mock putchar
static void minigdbstubUsrPutchar(char c) {
    g_mockPutcharBuf.push_back(c);
    return;
}

// --- Tests ---

TEST(minigdbstub, test_recvs) {
    for(auto packet : g_mockPackets) {
        g_packetChar = 0;
        g_mockPacket = packet;
        char expectChar = packet[2];
        gdbPacket gdbPkt;
        initDynCharBuffer(&gdbPkt.pktData, MINIGDBSTUB_PKT_SIZE);
        minigdbstubRecv(&gdbPkt);
        EXPECT_EQ(gdbPkt.commandType, expectChar);
        freeDynCharBuffer(&gdbPkt.pktData);
    }
}

// Send regs to mock GDB
TEST(minigdbstub, test_send_regs) {
    int regs[8] = {11,4,5,6,55,34,23,16};
    char *charRegs = (char*)regs;
    g_mockPutcharBuf.clear();
    minigdbstubSendRegs((char*)regs, sizeof(regs));
    for (int i=0; i<sizeof(regs); ++i) {
        char itoaBuff[3];
        HEX_ENCODE_ASCII(charRegs[i], itoaBuff);
        for (int j=0; j<2; ++j) {
            itoaBuff[j] = (itoaBuff[j] == 0) ? ('0') : (itoaBuff[j]);
        }
        if (itoaBuff[1] == '0') {
            EXPECT_EQ(itoaBuff[1], g_mockPutcharBuf[i*2]);
            EXPECT_EQ(itoaBuff[0], g_mockPutcharBuf[(i*2)+1]);
        }
        else {
            EXPECT_EQ(itoaBuff[0], g_mockPutcharBuf[i*2]);
            EXPECT_EQ(itoaBuff[1], g_mockPutcharBuf[(i*2)+1]);
        }
    }
}

TEST(minigdbstub, test_write_regs) {
    int regs[8] = {1,1,1,1,1,1,1,1};
    char *charRegs = (char*)regs;
    int expectedResults[8] = {11,4,5,6,55,34,23,16};
    char writeRegsData[] = {
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
    minigdbstubWriteRegs(writeRegsData, sizeof(writeRegsData), charRegs);
    for (int i=0; i<sizeof(regs)/sizeof(int); ++i) {
        EXPECT_EQ(expectedResults[i], regs[i]);
    }
}