#ifndef MINIGDBSTUB_H
#define MINIGDBSTUB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Basic dynamic char array utility for reading GDB packets
#define ALLOC_ASSERT(ptr) do { \
    if (ptr == NULL) { printf("[minigdbstub]: ERROR, could not allocate memory.\n"); } } while(0)
typedef struct {
    char *buffer;
    size_t used;
    size_t size;
} DynCharBuffer;
void initDynCharBuffer(DynCharBuffer *buf, size_t startSize) {
    buf->buffer = (char*)malloc(startSize * sizeof(char));
    ALLOC_ASSERT(buf->buffer);
    buf->used = 0;
    buf->size = startSize;
}
void insertDynCharBuffer(DynCharBuffer *buf, char item) {
    // Realloc if buffer is full - double the array size
    if (buf->used == buf->size) {
        buf->size *= 2;
        buf->buffer = (char*)realloc(buf->buffer, buf->size);
        ALLOC_ASSERT(buf->buffer);
    }
    buf->buffer[buf->used++] = item;
}
void freeDynCharBuffer(DynCharBuffer *buf) {
    free(buf->buffer);
    buf->buffer = NULL;
    buf->used = buf->size = 0;
}

// Useful GDB Remote Protocol Info
// - Packets are in the following form: $<packet-data>#<checksum>
// - Packet data is a sequence of chars (excluding "#" and "$")
// - Fields within the packet should be separated by either ',' or ';'
// - Unless noted, all numbers are represented in hex w/ leading zeros suppressed
// - Ack = "+"
// - Request retransmission = "-"

#define ACK_PACKET      "+$#00"
#define RESEND_PACKET   "-$#00"

// User stubs
static void minigdbstubUsrPutchar(char);
static char minigdbstubUsrGetchar(void);

int minigdbstubComputeChecksum(char *buffer, size_t len) {
    unsigned char checksum = 0;
    for (int i=0; i<len; ++i) {
        checksum += buffer[i];
    }
    return (int)checksum;
}

static void minigdbstubSend(const char *data) {
    for (int i=0; i<strlen(data); ++i) {
        minigdbstubUsrPutchar(data[i]);
    }
}

static char minigdbstubRecv(void) {
    char returnCommand;
    int goldChecksum, actualChecksum, currentOffset, checksumOffset;
    DynCharBuffer charBuf;
    initDynCharBuffer(&charBuf, 256);

    while (1) {
        currentOffset = checksumOffset = 0;

        // Get the beginning of the packet data '$'
        while (1) {
            char c = minigdbstubUsrGetchar();
            if (c == '$') {
                break;
            }
        }

        // Read packet data until the end '#' - then read the remaining 2 checksum digits
        while (1) {
            insertDynCharBuffer(&charBuf, minigdbstubUsrGetchar());
            if (charBuf.buffer[currentOffset] == '#') {
                ++currentOffset;
                checksumOffset = currentOffset;
                insertDynCharBuffer(&charBuf, minigdbstubUsrGetchar());
                insertDynCharBuffer(&charBuf, minigdbstubUsrGetchar());
                insertDynCharBuffer(&charBuf, 0);
                break;
            }
            ++currentOffset;
        }

        // Compute checksum and compare with expected checksum
        // Request retransmission if checksum verif. fails
        goldChecksum = strtol(&charBuf.buffer[checksumOffset], NULL, 16);
        actualChecksum = minigdbstubComputeChecksum(charBuf.buffer, checksumOffset-1);
        if (goldChecksum != actualChecksum) {
            charBuf.used = 0;
            minigdbstubSend(RESEND_PACKET);
            continue;
        }

        returnCommand = charBuf.buffer[0];
        // TODO: Process the packet data...

        minigdbstubSend(ACK_PACKET);
        freeDynCharBuffer(&charBuf);
        return returnCommand;
    }
}

void minigdbstubSendRegs(char *regs, size_t len) {
    // TODO: Implement...
}

void minigdbstubSendSignal(int signalNum) {
    DynCharBuffer *fmtPacket;
    fmtPacket = (DynCharBuffer*)malloc(sizeof(DynCharBuffer));
    initDynCharBuffer(fmtPacket, 32);
    insertDynCharBuffer(fmtPacket, '+');
    insertDynCharBuffer(fmtPacket, '$');
    insertDynCharBuffer(fmtPacket, 'S');

    // Convert signal num to hex char array
    char itoaBuff[20];
    snprintf(itoaBuff, sizeof(itoaBuff), "%x", signalNum);
    int i = 0;
    while (itoaBuff[i] != 0) {
        insertDynCharBuffer(fmtPacket, itoaBuff[i]);
        ++i;
    }
    insertDynCharBuffer(fmtPacket, '#');

    // Add the two checksum hex chars
    char checksumHex[4];
    int checksum = minigdbstubComputeChecksum(&fmtPacket->buffer[2], i+1);
    snprintf(checksumHex, sizeof(checksumHex), "%2x", checksum);
    insertDynCharBuffer(fmtPacket, checksumHex[0]);
    insertDynCharBuffer(fmtPacket, checksumHex[1]);

    minigdbstubSend((const char*)fmtPacket->buffer);
    freeDynCharBuffer(fmtPacket);
    free(fmtPacket);
}

// Main gdb stub process call
static void minigdbstubProcess(char *registersRaw, size_t registersLen, int signalNum) {
    char command;
    while (1) {
        // Poll from GDB until a packet is recieved
        command = minigdbstubRecv();

        switch(command) {
            case 'g':   // Read registers
                // TODO: Implement...
                minigdbstubSendRegs(registersRaw, registersLen);
                break;
            case 'G':   // Write registers
                // TODO: Implement...
                break;
            case 'p':   // Read one register
                // TODO: Implement...
                break;
            case 'P':   // Write one register
                // TODO: Implement...
                break;
            case 'm':   // Read mem
                // TODO: Implement...
                break;
            case 'M':   // Write mem
                // TODO: Implement...
                break;
            case 'X':   // Write mem (binary)
                // TODO: Implement...
                break;
            case 'c':   // Continue
                // TODO: Implement...
                return;
            case 's':   // Step
                // TODO: Implement...
                return;
            case 'F':   // File I/O extension
                // TODO: Implement...
                break;
            case '?':   // Indicate reason why target halted
                minigdbstubSendSignal(signalNum);
                break;
            default:    // Command unsupported
                // TODO: Implement...
                break;
        }
    }
}

#endif // MINIGDBSTUB_H