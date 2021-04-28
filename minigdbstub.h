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

// GDB Remote Serial Protocol packet object
typedef struct {
    char commandType;
    DynCharBuffer pktData;
    char checksum[3];
} gdbPacket;

// minigdbstub process call object
typedef struct {
    char    *regs;      // Pointer to register array
    size_t  regsSize;   // Size of register array in bytes
    size_t  regsCount;  // Total number of registers
    int     signalNum;  // Signal that can be sent to GDB on certain operations
    void    *usrData;   // Optional handle to opaque user data
} mgdbProcObj;

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

// Basic packets
#define ACK_PACKET      "+"
#define RESEND_PACKET   "-"
#define EMPTY_PACKET    "+$#00"
#define ERROR_PACKET    "+$E00#96"

#ifndef MINIGDBSTUB_PKT_SIZE
#define MINIGDBSTUB_PKT_SIZE 256
#endif

// Debugging macros
#ifdef MINIGDBSTUB_DEBUG
#define MINIGDBSTUB_LOG(format, ...) do { fprintf(stderr, "[minigdbstub]: " format, __VA_ARGS__); } while(0)
#else
#define MINIGDBSTUB_LOG(format, ...)
#endif

#define HEX_ENCODE_ASCII(in, len, out) snprintf(out, len, "%x", in);
#define HEX_DECODE_ASCII(in, out) out = strtol(in, NULL, 16);

// User stubs
static void minigdbstubUsrPutchar(char data, void *usrData);
static char minigdbstubUsrGetchar(void *usrData);
static void minigdbstubUsrWriteMem(size_t addr, unsigned char data, void *usrData);
static unsigned char minigdbstubUsrReadMem(size_t addr, void *usrData);
static void minigdbstubUsrContinue(void *usrData);
static void minigdbstubUsrStep(void *usrData);

static void minigdbstubComputeChecksum(char *buffer, size_t len, char *outBuf) {
    unsigned char checksum = 0;
    for (size_t i=0; i<len; ++i) {
        checksum += buffer[i];
    }
    HEX_ENCODE_ASCII(checksum, 3, outBuf);
}

static void minigdbstubSend(const char *data, void *usrData) {
    MINIGDBSTUB_LOG("GDB <--- minigdbstub : packet = %s\n", data);
    for (size_t i=0; i<strlen(data); ++i) {
        minigdbstubUsrPutchar(data[i], usrData);
    }
}

static void minigdbstubRecv(mgdbProcObj *mgdbObj, gdbPacket *gdbPkt) {
    int currentOffset = 0;
    char actualChecksum[3];
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
        // Request retransmission if checksum verif. fails
        minigdbstubComputeChecksum(gdbPkt->pktData.buffer, currentOffset, actualChecksum);
        if (strcmp(gdbPkt->checksum, actualChecksum) != 0) {
            gdbPkt->pktData.used = 0;
            minigdbstubSend(RESEND_PACKET, mgdbObj->usrData);
            continue;
        }

        gdbPkt->commandType = gdbPkt->pktData.buffer[0];
        MINIGDBSTUB_LOG("GDB ---> minigdbstub : packet = %s%s%s%s\n", 
            "$", gdbPkt->pktData.buffer, "#", gdbPkt->checksum);
        minigdbstubSend(ACK_PACKET, mgdbObj->usrData);
        return;
    }
}

static void minigdbstubWriteRegs(mgdbProcObj *mgdbObj, gdbPacket *recvPkt) {
    char tmpBuf[3] = {0, 0, 0};
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

    for (size_t i=0; i<mgdbObj->regsSize; ++i) {
        char itoaBuff[3];
        HEX_ENCODE_ASCII(mgdbObj->regs[i], 3, itoaBuff);
        for (int j=0; j<2; ++j) {
            itoaBuff[j] = (itoaBuff[j] == 0) ? ('0') : (itoaBuff[j]);
        }
        if (itoaBuff[1] == '0') {
            insertDynCharBuffer(&sendPkt, itoaBuff[1]);
            insertDynCharBuffer(&sendPkt, itoaBuff[0]);
        }
        else {
            insertDynCharBuffer(&sendPkt, itoaBuff[0]);
            insertDynCharBuffer(&sendPkt, itoaBuff[1]);
        }
    }
    insertDynCharBuffer(&sendPkt, 0);

    minigdbstubSend((const char*)sendPkt.buffer, mgdbObj->usrData);
    freeDynCharBuffer(&sendPkt);
}

static void minigdbstubSendReg(mgdbProcObj *mgdbObj, gdbPacket *recvPkt) {
    int index;
    size_t regWidth = mgdbObj->regsSize / mgdbObj->regsCount;
    HEX_DECODE_ASCII(&recvPkt->pktData.buffer[1], index);
    
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
        char atoiBuf[3] = {0,0,0};
        atoiBuf[0] = recvPkt->pktData.buffer[valOffset+(i*2)];
        atoiBuf[1] = recvPkt->pktData.buffer[valOffset+(i*2)+1];
        HEX_DECODE_ASCII(atoiBuf, decodedVal);
        minigdbstubUsrWriteMem(address+i, (unsigned char)decodedVal, mgdbObj->usrData);
    }

    // Send ACK to GDB
    minigdbstubSend(ACK_PACKET, mgdbObj->usrData);
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

    // Call user read memory handler
    for (size_t i=0; i<length; ++i) {
        char itoaBuff[3];
        unsigned char c = minigdbstubUsrReadMem(address+i, mgdbObj->usrData);
        HEX_ENCODE_ASCII(c, 3, itoaBuff);
        for (int j=0; j<2; ++j) {
            itoaBuff[j] = (itoaBuff[j] == 0) ? ('0') : (itoaBuff[j]);
        }
        if (itoaBuff[1] == '0') {
            insertDynCharBuffer(&memBuf, itoaBuff[1]);
            insertDynCharBuffer(&memBuf, itoaBuff[0]);
        }
        else {
            insertDynCharBuffer(&memBuf, itoaBuff[0]);
            insertDynCharBuffer(&memBuf, itoaBuff[1]);
        }
    }

    minigdbstubSend((const char*)memBuf.buffer, mgdbObj->usrData);
    freeDynCharBuffer(&memBuf);
}

static void minigdbstubSendSignal(mgdbProcObj *mgdbObj) {
    DynCharBuffer sendPkt;
    initDynCharBuffer(&sendPkt, 32);
    insertDynCharBuffer(&sendPkt, '+');
    insertDynCharBuffer(&sendPkt, '$');
    insertDynCharBuffer(&sendPkt, 'S');

    // Convert signal num to hex char array
    char itoaBuff[20];
    HEX_ENCODE_ASCII(mgdbObj->signalNum, 20, itoaBuff);
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

    minigdbstubSend((const char*)sendPkt.buffer, mgdbObj->usrData);
    freeDynCharBuffer(&sendPkt);
}

// Main gdb stub process call
static void minigdbstubProcess(mgdbProcObj *mgdbObj) {
    gdbPacket recvPkt;
    initDynCharBuffer(&recvPkt.pktData, MINIGDBSTUB_PKT_SIZE);
    
    while (1) {
        // Poll from GDB until a packet is recieved
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
            case 'm':   {   // Read mem - TODO: Endianess???
                minigdbstubReadMem(mgdbObj, &recvPkt);
                break;
            }
            case 'M':   {   // Write mem - TODO: Endianess???
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
            case '?':   {   // Indicate reason why target halted
                minigdbstubSendSignal(mgdbObj);
                break;;
            }
            default:    {   // Command unsupported
                minigdbstubSend(EMPTY_PACKET, mgdbObj->usrData);
                break;
            }
        }
    }

    // Cleanup packet mem
    freeDynCharBuffer(&recvPkt.pktData);
}

#endif // MINIGDBSTUB_H
