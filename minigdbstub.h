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

// GDB Remote Serial Protocol packet object
typedef struct {
    char commandType;
    DynCharBuffer pktData;
    char checksum[3];
} gdbPacket;

// Useful GDB Remote Protocol Info
// - Packets are in the following form: $<packet-data>#<checksum>
// - Packet data is a sequence of chars (excluding "#" and "$")
// - Fields within the packet should be separated by either ',' or ';'
// - Unless noted, all numbers are represented in hex w/ leading zeros suppressed
// - Ack = "+"
// - Request retransmission = "-"
//
// --- A few relevant send/recieve packets ---
// - 'Z0/z0,addr,kind' = [Insert/Remove] software breakpoint at 'addr' of type 'kind'
// - 'Z1/z1,addr,kind' = [Insert/Remove] hardware breakpoint at 'addr' of type 'kind'
// - 'm/Maddr,length'  = [Read/Write] length addressable memory units starting at addr
//      - Replies:
//          - 'xxx...xx'    = Memory contents in hex
//          - 'Enn'         = Error where 'nn' is the error number
// - 'Odata' = Tell GDB to decode the hex 'data' field to ASCII and print to GDB console

#define ACK_PACKET      "+"
#define RESEND_PACKET   "-"
#define EMPTY_PACKET    "+$#00"
#define ERROR_PACKET    "+$E00#96"

#ifndef MINIGDBSTUB_PKT_SIZE
#define MINIGDBSTUB_PKT_SIZE 256
#endif

#define HEX_ENCODE_ASCII(in, out) snprintf(out, sizeof(out), "%x", in);
#define HEX_DECODE_ASCII(in, out) out = strtol(in, NULL, 16);

// User stubs
static void minigdbstubUsrPutchar(char);
static char minigdbstubUsrGetchar(void);

void minigdbstubComputeChecksum(char *buffer, size_t len, char *outBuf) {
    unsigned char checksum = 0;
    for (int i=0; i<len; ++i) {
        checksum += buffer[i];
    }
    HEX_ENCODE_ASCII(checksum, outBuf);
}

static void minigdbstubSend(const char *data) {
    for (int i=0; i<strlen(data); ++i) {
        minigdbstubUsrPutchar(data[i]);
    }
}

static void minigdbstubRecv(gdbPacket *gdbPkt) {
    int currentOffset = 0;
    char actualChecksum[3];
    char c;
    while (1) {
        // Get the beginning of the packet data '$'
        while (1) {
            c = minigdbstubUsrGetchar();
            if (c == '$') {
                break;
            }
        }

        // Read packet data until the end '#' - then read the remaining 2 checksum digits
        while (1) {
            c = minigdbstubUsrGetchar();
            if (c == '#') {
                gdbPkt->checksum[0] = minigdbstubUsrGetchar();
                gdbPkt->checksum[1] = minigdbstubUsrGetchar();
                gdbPkt->checksum[2] = 0;
                insertDynCharBuffer(&gdbPkt->pktData, 0);
                break;
            }
            insertDynCharBuffer(&gdbPkt->pktData, c);
            ++currentOffset;
        }

        // Compute checksum and compare with expected checksum
        // Request retransmission if checksum verif. fails
        minigdbstubComputeChecksum(gdbPkt->pktData.buffer, currentOffset, actualChecksum);
        if (strcmp(gdbPkt->checksum, actualChecksum) != 0) {
            gdbPkt->pktData.used = 0;
            minigdbstubSend(RESEND_PACKET);
            continue;
        }

        gdbPkt->commandType = gdbPkt->pktData.buffer[0];
        minigdbstubSend(ACK_PACKET);
        return;
    }
}

void minigdbstubSendRegs(char *regs, size_t len) {
    DynCharBuffer sendPkt;
    initDynCharBuffer(&sendPkt, 512);
    insertDynCharBuffer(&sendPkt, '+');
    insertDynCharBuffer(&sendPkt, '$');

    for (int i=0; i<len; ++i) {
        char itoaBuff[3];
        HEX_ENCODE_ASCII(regs[i] & 0xff, itoaBuff);
        insertDynCharBuffer(&sendPkt, itoaBuff[0]);
    }

    // Add the two checksum hex chars
    char checksumHex[3];
    minigdbstubComputeChecksum(&sendPkt.buffer[2], len, checksumHex);
    insertDynCharBuffer(&sendPkt, checksumHex[0]);
    insertDynCharBuffer(&sendPkt, checksumHex[1]);

    minigdbstubSend((const char*)sendPkt.buffer);
    freeDynCharBuffer(&sendPkt);
}

void minigdbstubSendSignal(int signalNum) {
    DynCharBuffer sendPkt;
    initDynCharBuffer(&sendPkt, 32);
    insertDynCharBuffer(&sendPkt, '+');
    insertDynCharBuffer(&sendPkt, '$');
    insertDynCharBuffer(&sendPkt, 'S');

    // Convert signal num to hex char array
    char itoaBuff[20];
    HEX_ENCODE_ASCII(signalNum, itoaBuff);
    int i = 0;
    while (itoaBuff[i] != 0) {
        insertDynCharBuffer(&sendPkt, itoaBuff[i]);
        ++i;
    }
    insertDynCharBuffer(&sendPkt, '#');

    // Add the two checksum hex chars
    char checksumHex[3];
    minigdbstubComputeChecksum(&sendPkt.buffer[2], i+1, checksumHex);
    insertDynCharBuffer(&sendPkt, checksumHex[0]);
    insertDynCharBuffer(&sendPkt, checksumHex[1]);
    insertDynCharBuffer(&sendPkt, 0);

    minigdbstubSend((const char*)sendPkt.buffer);
    freeDynCharBuffer(&sendPkt);
}

// Main gdb stub process call
static void minigdbstubProcess(char *registersRaw, size_t registersLen, int signalNum, void *usrData) {
    gdbPacket recvPkt;
    initDynCharBuffer(&recvPkt.pktData, MINIGDBSTUB_PKT_SIZE);
    
    while (1) {
        // Poll from GDB until a packet is recieved
        minigdbstubRecv(&recvPkt);

        switch(recvPkt.commandType) {
            case 'g':   {   // Read registers
                minigdbstubSendRegs(registersRaw, registersLen);
                break;
            }
            case 'G':   {   // Write registers
                char tmpBuf[3] = {0, 0, 0};
                int decodedHex;
                for (int i=0; i<recvPkt.pktData.used-1; ++i) {
                    tmpBuf[i%2] = recvPkt.pktData.buffer[i+1];
                    if ((i%2) != 0) {
                        HEX_DECODE_ASCII(tmpBuf, decodedHex);
                        registersRaw[i/2] = (char)decodedHex;
                    }
                }
                return;
            }
            case 'p':   {   // Read one register
                // TODO: Implement...
                break;
            }
            case 'P':   {   // Write one register
                // TODO: Implement...
                break;
            }
            case 'm':   {   // Read mem
                // TODO: Implement...
                break;
            }
            case 'M':   {   // Write mem
                // TODO: Implement...
                break;
            }
            case 'X':   {   // Write mem (binary)
                // TODO: Implement...
                break;
            }
            case 'c':   {   // Continue
                // TODO: Implement...
                return;
            }
            case 's':   {   // Step
                // TODO: Implement...
                return;
            }
            case '?':   {   // Indicate reason why target halted
                minigdbstubSendSignal(signalNum);
                break;
            }
            default:    {   // Command unsupported
                // TODO: Implement...
                break;
            }
        }
    }

    // Cleanup packet mem
    freeDynCharBuffer(&recvPkt.pktData);
}

#endif // MINIGDBSTUB_H