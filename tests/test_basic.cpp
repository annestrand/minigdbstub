#include <stdio.h>

#include "minigdbstub.h"
#include "gtest/gtest.h"

// Mock getchar
static char minigdbstubUsrGetchar(void) {
    const char *mockPacket = "$g#67";
    static int i = 0;
    return mockPacket[i++];
}

TEST(minigdbstub, basic_recv) {
    char c = minigdbstubRecv();
    EXPECT_EQ(c, 'g');
}