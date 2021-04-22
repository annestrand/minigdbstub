#include "criterion.h"
#include "minigdbstub.h"
#include <stdio.h>

// Mock getchar
static char minigdbstubUsrGetchar(void) {
    const char *mockPacket = "$g#67";
    static int i = 0;
    return mockPacket[i++];
}

Test(minigdbstub, basic_recv) {
    char c = minigdbstubRecv();
    cr_assert('g' == 'g');
}