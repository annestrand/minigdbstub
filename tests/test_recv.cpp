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
    "+$c#63"
};
unsigned int g_packetChar;
const char *g_mockPacket;

// Mock getchar
static char minigdbstubUsrGetchar(void) {
    return g_mockPacket[g_packetChar++];
}

// Mock putchar
static void minigdbstubUsrPutchar(char c) {
    GTEST_COUT << "Recieved char: " << c << std::endl;
    return;
}

// --- Tests ---

TEST(minigdbstub, basic_recv) {
    for(auto packet : g_mockPackets) {
        g_packetChar = 0;
        g_mockPacket = packet;
        char expectChar = packet[2];
        char c = minigdbstubRecv();
        EXPECT_EQ(c, expectChar);
    }
}