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
static void initDynCharBuffer(DynCharBuffer *buf, size_t startSize) {
    buf->buffer = (char*)malloc(startSize * sizeof(char));
    ALLOC_ASSERT(buf->buffer);
    buf->used = 0;
    buf->size = startSize;
}
static void insertDynCharBuffer(DynCharBuffer *buf, char item) {
    // Realloc if buffer is full - double the array size
    if (buf->used == buf->size) {
        buf->size *= 2;
        buf->buffer = (char*)realloc(buf->buffer, buf->size);
        ALLOC_ASSERT(buf->buffer);
    }
    buf->buffer[buf->used++] = item;
}
static void freeDynCharBuffer(DynCharBuffer *buf) {
    free(buf->buffer);
    buf->buffer = NULL;
    buf->used = buf->size = 0;
}

// Basic packets
#define ACK_PACKET      "+"
#define RESEND_PACKET   "-"
#define EMPTY_PACKET    "$#00"
#define ERROR_PACKET    "$E00#96"
#define OK_PACKET       "$OK#9a"

#ifndef MINIGDBSTUB_PKT_SIZE
#define MINIGDBSTUB_PKT_SIZE 256
#endif

#define MINIGDBSTUB_LOG(format, ...) do { fprintf(stderr, "[minigdbstub]: " format, __VA_ARGS__); } while(0)

#define HEX_DECODE_ASCII(in, out) out = strtol(in, NULL, 16)
#define HEX_ENCODE_ASCII(in, len, out) snprintf(out, len, "%x", in)
#define DEC_ENCODE_ASCII(in, len, out) snprintf(out, len, "%d", in)

// GDB Remote Serial Protocol packet object
typedef struct {
    char commandType;
    DynCharBuffer pktData;
    char checksum[3];
} gdbPacket;

// minigdbstub option-bits struct
typedef struct {
    unsigned int o_signalOnEntry : 1;
    unsigned int o_enableLogging : 1;
} mgdbOpts;

enum {
    MGDB_SOFT_BREAKPOINT =  (1<<0),
    MGDB_HARD_BREAKPOINT =  (1<<1),

    MGDB_SET_BREAKPOINT =   (1<<2),
    MGDB_CLEAR_BREAKPOINT = (1<<3)
};

// minigdbstub process call object
typedef struct {
    char        *regs;      // Pointer to register array
    size_t      regsSize;   // Size of register array in bytes
    size_t      regsCount;  // Total number of registers
    int         signalNum;  // Signal that can be sent to GDB on certain operations
    mgdbOpts    opts;       // Options bitfield
    void        *usrData;   // Optional handle to opaque user data
} mgdbProcObj;

// User stubs
static void minigdbstubUsrPutchar(char data, void *usrData);
static char minigdbstubUsrGetchar(void *usrData);
static void minigdbstubUsrWriteMem(size_t addr, unsigned char data, void *usrData);
static unsigned char minigdbstubUsrReadMem(size_t addr, void *usrData);
static void minigdbstubUsrContinue(void *usrData);
static void minigdbstubUsrStep(void *usrData);
static void minigdbstubUsrProcessBreakpoint(int type, size_t addr, void *usrData);

static void minigdbstubComputeChecksum(char *buffer, size_t len, char *outBuf) {
    unsigned int checksum = 0;
    for (size_t i=0; i<len; ++i) {
        checksum = (checksum + buffer[i]) % 256;
    }
    HEX_ENCODE_ASCII(checksum, 3, outBuf);

    // If the value is single digit
    if (outBuf[1] == 0) {
        outBuf[1] = outBuf[0];
        outBuf[0] = '0';
    }
}

static void minigdbstubSend(const char *data, mgdbProcObj *mgdbObj) {
    size_t len = strlen(data);
    if (mgdbObj->opts.o_enableLogging) {
        MINIGDBSTUB_LOG("GDB <--- minigdbstub : packet = %s\n", data);
    }
    for (size_t i=0; i<len; ++i) {
        minigdbstubUsrPutchar(data[i], mgdbObj->usrData);
    }
}

static void minigdbstubRecv(mgdbProcObj *mgdbObj, gdbPacket *gdbPkt) {
    int currentOffset = 0;
    char c;
    while (1) {
        // Get the beginning of the packet data '$'
        while (1) {
            c = minigdbstubUsrGetchar(mgdbObj->usrData);
            if (c == '$') {
                break;
            }
        }

        // Read packet data until the end '#' - then read the remaining 2 checksum digits
        while (1) {
            c = minigdbstubUsrGetchar(mgdbObj->usrData);
            if (c == '#') {
                gdbPkt->checksum[0] = minigdbstubUsrGetchar(mgdbObj->usrData);
                gdbPkt->checksum[1] = minigdbstubUsrGetchar(mgdbObj->usrData);
                gdbPkt->checksum[2] = 0;
                insertDynCharBuffer(&gdbPkt->pktData, 0);
                break;
            }
            insertDynCharBuffer(&gdbPkt->pktData, c);
            ++currentOffset;
        }

        // Compute checksum and compare with expected checksum
        // Request retransmission if checksum verification fails
        char actualChecksum[8] = {0,0,0,0,0,0,0,0};
        minigdbstubComputeChecksum(gdbPkt->pktData.buffer, currentOffset, actualChecksum);
        if (strcmp(gdbPkt->checksum, actualChecksum) != 0) {
            gdbPkt->pktData.used = 0;
            minigdbstubSend(RESEND_PACKET, mgdbObj);
            continue;
        }

        gdbPkt->commandType = gdbPkt->pktData.buffer[0];
        if (mgdbObj->opts.o_enableLogging) {
            MINIGDBSTUB_LOG("GDB ---> minigdbstub : packet = %s%s%s%s\n",
                "$", gdbPkt->pktData.buffer, "#", gdbPkt->checksum);
        }
        minigdbstubSend(ACK_PACKET, mgdbObj);
        return;
    }
}

static void minigdbstubWriteRegs(mgdbProcObj *mgdbObj, gdbPacket *recvPkt) {
    char tmpBuf[8];
    int decodedHex;
    for (size_t i=0; i<recvPkt->pktData.size-1; ++i) {
        tmpBuf[i%2] = recvPkt->pktData.buffer[i];
        if ((i%2) != 0) {
            HEX_DECODE_ASCII(tmpBuf, decodedHex);
            mgdbObj->regs[i/2] = (char)decodedHex;
        }
    }
}

static void minigdbstubWriteReg(mgdbProcObj *mgdbObj, gdbPacket *recvPkt) {
    int index, valOffset = 0;
    size_t regWidth = mgdbObj->regsSize / mgdbObj->regsCount;
    for (int i=0; recvPkt->pktData.buffer[i] != 0; ++i) {
        if (recvPkt->pktData.buffer[i] == '=') {
            recvPkt->pktData.buffer[i] = 0;
            ++valOffset;
            break;
        }
        ++valOffset;
    }
    HEX_DECODE_ASCII(&recvPkt->pktData.buffer[1], index);

    mgdbProcObj tmpProcObj = *mgdbObj;
    tmpProcObj.regsCount = 1;
    tmpProcObj.regs = &mgdbObj->regs[index * regWidth];
    tmpProcObj.regsSize = regWidth;

    gdbPacket tmpRecPkt = *recvPkt;
    tmpRecPkt.pktData.buffer = &recvPkt->pktData.buffer[valOffset];

    minigdbstubWriteRegs(&tmpProcObj, &tmpRecPkt);
}

static void minigdbstubSendRegs(mgdbProcObj *mgdbObj) {
    DynCharBuffer sendPkt;
    initDynCharBuffer(&sendPkt, 512);
    insertDynCharBuffer(&sendPkt, '$');

    for (size_t i=0; i<mgdbObj->regsSize; ++i) {
        char itoaBuff[8];
        HEX_ENCODE_ASCII((unsigned char)mgdbObj->regs[i], 3, itoaBuff);

        // Swap if single digit
        if (itoaBuff[1] == 0) {
            itoaBuff[1] = itoaBuff[0];
            itoaBuff[0] = '0';
        }

        insertDynCharBuffer(&sendPkt, itoaBuff[0]);
        insertDynCharBuffer(&sendPkt, itoaBuff[1]);
    }

    // Compute and append the checksum
    char checksum[8];
    size_t len = sendPkt.used - 1;
    minigdbstubComputeChecksum(&sendPkt.buffer[1], len, checksum);
    insertDynCharBuffer(&sendPkt, '#');
    insertDynCharBuffer(&sendPkt, checksum[0]);
    insertDynCharBuffer(&sendPkt, checksum[1]);
    insertDynCharBuffer(&sendPkt, 0);

    minigdbstubSend((const char*)sendPkt.buffer, mgdbObj);
    freeDynCharBuffer(&sendPkt);
}

static void minigdbstubSendReg(mgdbProcObj *mgdbObj, gdbPacket *recvPkt) {
    int index;
    size_t regWidth = mgdbObj->regsSize / mgdbObj->regsCount;
    HEX_DECODE_ASCII(&recvPkt->pktData.buffer[2], index);
    
    mgdbProcObj tmpProcObj = *mgdbObj;
    tmpProcObj.regsCount = 1;
    tmpProcObj.regs = &mgdbObj->regs[index * regWidth];
    tmpProcObj.regsSize = regWidth;
    
    minigdbstubSendRegs(&tmpProcObj);
}

static void minigdbstubWriteMem(mgdbProcObj *mgdbObj, gdbPacket *recvPkt) {
    size_t address, length;
    int lengthOffset = 0;
    int valOffset = 0;

    // Grab the length offset
    for (int i=0; recvPkt->pktData.buffer[i] != 0; ++i) {
        if ((recvPkt->pktData.buffer[i] == ',')     ||
                (recvPkt->pktData.buffer[i] == ';') ||
                    (recvPkt->pktData.buffer[i] == ':')) {
            recvPkt->pktData.buffer[i] = 0;
            ++lengthOffset;
            valOffset = lengthOffset;
            break;
        }
        ++lengthOffset;
    }
    // Grab the data value offset
    for (int i=0; recvPkt->pktData.buffer[i] != 0; ++i) {
        if ((recvPkt->pktData.buffer[i] == ',')     ||
                (recvPkt->pktData.buffer[i] == ';') ||
                    (recvPkt->pktData.buffer[i] == ':')) {
            recvPkt->pktData.buffer[i] = 0;
            ++valOffset;
            break;
        }
        ++valOffset;
    }
    HEX_DECODE_ASCII(&recvPkt->pktData.buffer[1], address);
    HEX_DECODE_ASCII(&recvPkt->pktData.buffer[lengthOffset], length);

    // Call user write memory handler
    for (size_t i=0; i<length; ++i) {
        int decodedVal;
        char atoiBuf[8];
        atoiBuf[0] = recvPkt->pktData.buffer[valOffset+(i*2)];
        atoiBuf[1] = recvPkt->pktData.buffer[valOffset+(i*2)+1];
        HEX_DECODE_ASCII(atoiBuf, decodedVal);
        minigdbstubUsrWriteMem(address+i, (unsigned char)decodedVal, mgdbObj->usrData);
    }

    // Send ACK to GDB
    minigdbstubSend(ACK_PACKET, mgdbObj);
}

static void minigdbstubReadMem(mgdbProcObj *mgdbObj, gdbPacket *recvPkt) {
    size_t address, length;
    int valOffset = 0;
    for (int i=0; recvPkt->pktData.buffer[i] != 0; ++i) {
        if ((recvPkt->pktData.buffer[i] == ',')     ||
                (recvPkt->pktData.buffer[i] == ';') ||
                    (recvPkt->pktData.buffer[i] == ':')) {
            recvPkt->pktData.buffer[i] = 0;
            ++valOffset;
            break;
        }
        ++valOffset;
    }
    HEX_DECODE_ASCII(&recvPkt->pktData.buffer[1], address);
    HEX_DECODE_ASCII(&recvPkt->pktData.buffer[valOffset], length);

    // Alloc a packet w/ the requested data to send as a response to GDB
    DynCharBuffer memBuf;
    initDynCharBuffer(&memBuf, length);
    insertDynCharBuffer(&memBuf, '$');

    // Call user read memory handler
    for (size_t i=0; i<length; ++i) {
        char itoaBuff[8];
        unsigned char c = minigdbstubUsrReadMem(address+i, mgdbObj->usrData);
        HEX_ENCODE_ASCII(c, 3, itoaBuff);

        // Swap if single digit
        if (itoaBuff[1] == 0) {
            itoaBuff[1] = itoaBuff[0];
            itoaBuff[0] = '0';
        }

        insertDynCharBuffer(&memBuf, itoaBuff[0]);
        insertDynCharBuffer(&memBuf, itoaBuff[1]);
    }

    // Compute and append the checksum
    char checksum[8];
    size_t len = memBuf.used - 1;
    minigdbstubComputeChecksum(&memBuf.buffer[1], len, checksum);
    insertDynCharBuffer(&memBuf, '#');
    insertDynCharBuffer(&memBuf, checksum[0]);
    insertDynCharBuffer(&memBuf, checksum[1]);
    insertDynCharBuffer(&memBuf, 0);

    minigdbstubSend((const char*)memBuf.buffer, mgdbObj);
    freeDynCharBuffer(&memBuf);
}

static void minigdbstubSendSignal(mgdbProcObj *mgdbObj) {
    DynCharBuffer sendPkt;
    initDynCharBuffer(&sendPkt, 32);
    insertDynCharBuffer(&sendPkt, '+');
    insertDynCharBuffer(&sendPkt, '$');
    insertDynCharBuffer(&sendPkt, 'S');

    // Convert signal num to hex char array
    char itoaBuff[8] = {0,0,0,0,0,0,0,0};
    DEC_ENCODE_ASCII(mgdbObj->signalNum, 8, itoaBuff);

    // Swap if single digit
    if (itoaBuff[1] == 0) {
        itoaBuff[1] = itoaBuff[0];
        itoaBuff[0] = '0';
    }

    int bufferPtr = 0;
    while (itoaBuff[bufferPtr] != 0) {
        insertDynCharBuffer(&sendPkt, itoaBuff[bufferPtr]);
        ++bufferPtr;
    }
    insertDynCharBuffer(&sendPkt, '#');

    // Add the two checksum hex chars
    char checksumHex[8] = {0,0,0,0,0,0,0,0};
    minigdbstubComputeChecksum(&sendPkt.buffer[2], bufferPtr+1, checksumHex);
    insertDynCharBuffer(&sendPkt, checksumHex[0]);
    insertDynCharBuffer(&sendPkt, checksumHex[1]);
    insertDynCharBuffer(&sendPkt, 0);

    minigdbstubSend((const char*)sendPkt.buffer, mgdbObj);
    freeDynCharBuffer(&sendPkt);
}

static void minigdbstubProcessBreakpoint(mgdbProcObj *mgdbObj, gdbPacket *recvPkt, int type) {
    int offset = 0;
    size_t address;
    for (int i=0; recvPkt->pktData.buffer[i] != 0; ++i) {
        if ((recvPkt->pktData.buffer[i] == ',')     ||
                (recvPkt->pktData.buffer[i] == ';') ||
                    (recvPkt->pktData.buffer[i] == ':')) {
            if (offset > 0) {
                recvPkt->pktData.buffer[i] = 0;
                break;
            }
            offset = i+1;
        }
    }
    HEX_DECODE_ASCII(&recvPkt->pktData.buffer[offset], address);

    switch (recvPkt->pktData.buffer[2]) {
        case '0': // Software breakpoint
            type |= MGDB_SOFT_BREAKPOINT;
            break;
        case '1': // Hardware breakpoint
            type |= MGDB_HARD_BREAKPOINT;
            break;
        default: // Other breakpoint/watchpoint type (unsupported)
            break;
    }
    minigdbstubUsrProcessBreakpoint(type, address, mgdbObj->usrData);

    // Send ACK + OK to GDB
    minigdbstubSend(ACK_PACKET, mgdbObj);
    minigdbstubSend(OK_PACKET, mgdbObj);
}

// Main gdb stub process call
static void minigdbstubProcess(mgdbProcObj *mgdbObj) {
    if (mgdbObj->opts.o_signalOnEntry) {
            minigdbstubSendSignal(mgdbObj);
    }

    // Poll and reply to packets from GDB until exit-related command
    while (1) {
        gdbPacket recvPkt;
        initDynCharBuffer(&recvPkt.pktData, MINIGDBSTUB_PKT_SIZE);
        minigdbstubRecv(mgdbObj, &recvPkt);

        switch(recvPkt.commandType) {
            case 'g':   {   // Read registers
                minigdbstubSendRegs(mgdbObj);
                break;
            }
            case 'G':   {   // Write registers
                minigdbstubWriteRegs(mgdbObj, &recvPkt);
                break;
            }
            case 'p':   {   // Read one register
                minigdbstubSendReg(mgdbObj, &recvPkt);
                break;
            }
            case 'P':   {   // Write one register
                minigdbstubWriteReg(mgdbObj, &recvPkt);
                break;
            }
            case 'm':   {   // Read mem
                minigdbstubReadMem(mgdbObj, &recvPkt);
                break;
            }
            case 'M':   {   // Write mem
                minigdbstubWriteMem(mgdbObj, &recvPkt);
                break;
            }
            case 'c':   {   // Continue
                minigdbstubUsrContinue(mgdbObj->usrData);
                return;
            }
            case 's':   {   // Step
                minigdbstubUsrStep(mgdbObj->usrData);
                return;
            }
            case 'Z':   {   // Place breakpoint
                minigdbstubProcessBreakpoint(mgdbObj, &recvPkt, MGDB_SET_BREAKPOINT);
                break;
            }
            case 'z':   {   // Remove breakpoint
                minigdbstubProcessBreakpoint(mgdbObj, &recvPkt, MGDB_CLEAR_BREAKPOINT);
                break;
            }
            case '?':   {   // Indicate reason why target halted
                minigdbstubSendSignal(mgdbObj);
                break;;
            }
            default:    {   // Command unsupported
                minigdbstubSend(EMPTY_PACKET, mgdbObj);
                break;
            }
        }

        // Cleanup packet mem
        freeDynCharBuffer(&recvPkt.pktData);
    }
}

#endif // MINIGDBSTUB_H
