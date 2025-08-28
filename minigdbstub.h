#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Basic packets
#define MGDB_ACK_PACKET "+"
#define MGDB_RESEND_PACKET "-"
#define MGDB_EMPTY_PACKET "$#00"
#define MGDB_ERROR_PACKET "$E00#96"
#define MGDB_OK_PACKET "$OK#9a"

#ifndef MGDB_PKT_SIZE
#    define MGDB_PKT_SIZE 256
#endif

// Log/trace/debug macros
#define MGDB_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define MGDB_LOG_I(msg, ...)                                                                  \
    printf("[minigdbstub] INFO  [ %12s:%-6d ]%20s : " msg, MGDB_FILENAME, __LINE__, __func__, \
           ##__VA_ARGS__)
#define MGDB_LOG_W(msg, ...)                                                                  \
    printf("[minigdbstub] WARN  [ %12s:%-6d ]%20s : " msg, MGDB_FILENAME, __LINE__, __func__, \
           ##__VA_ARGS__)
#define MGDB_LOG_E(msg, ...)                                                                  \
    printf("[minigdbstub] ERROR [ %12s:%-6d ]%20s : " msg, MGDB_FILENAME, __LINE__, __func__, \
           ##__VA_ARGS__)
#define MGDB_LOG_TRACE(msg, ...) printf("[minigdbstub] : " msg, ##__VA_ARGS__)
#define MGDB_SEND "GDB <--- MGDB_STUB"
#define MGDB_RECV "GDB ---> MGDB_STUB"

#define MGDB_HEX_DECODE_ASCII(in, out) out = strtol(in, NULL, 16)
#define MGDB_HEX_ENCODE_ASCII(in, len, out) snprintf(out, len, "%x", in)
#define MGDB_DEC_ENCODE_ASCII(in, len, out) snprintf(out, len, "%d", in)

// Set if error and return early
#define MGDB_CHECK_RET(ret, mgdbObj)      \
    do                                    \
    {                                     \
        mgdbObj->err = ret;               \
        if (mgdbObj->err != MGDB_SUCCESS) \
        {                                 \
            return;                       \
        }                                 \
    } while (0)

enum
{
    MGDB_SUCCESS,
    MGDB_ALLOC_FAILED
};

// Basic dynamic char array utility for reading GDB packets
typedef struct
{
    char *buffer;
    size_t used;
    size_t size;
} DynCharBuffer;

static int initDynCharBuffer(DynCharBuffer *buf, size_t startSize)
{
    buf->buffer = (char *)malloc(startSize * sizeof(char));
    if (buf->buffer == NULL)
    {
        MGDB_LOG_E("Failed to alloc memory!\n");
        return MGDB_ALLOC_FAILED;
    }
    buf->used = 0;
    buf->size = startSize;
    return MGDB_SUCCESS;
}
static int insertDynCharBuffer(DynCharBuffer *buf, char item)
{
    // Realloc if buffer is full - double the array size
    if (buf->used == buf->size)
    {
        buf->size *= 2;
        buf->buffer = (char *)realloc(buf->buffer, buf->size);
        if (buf->buffer == NULL)
        {
            MGDB_LOG_E("Failed to realloc memory!\n");
            return MGDB_ALLOC_FAILED;
        }
    }
    buf->buffer[buf->used++] = item;
    return MGDB_SUCCESS;
}
static void freeDynCharBuffer(DynCharBuffer *buf)
{
    free(buf->buffer);
    buf->buffer = NULL;
    buf->used = buf->size = 0;
}

// GDB Remote Serial Protocol packet object s
typedef struct
{
    char commandType;
    DynCharBuffer pktData;
    char checksum[3];
} gdbPacket;

// minigdbstub option-bits struct
typedef struct
{
    unsigned int o_signalOnEntry : 1;
    unsigned int o_enableLogging : 1;
} mgdbOpts;

enum
{
    MGDB_SOFT_BREAKPOINT = (1 << 0),
    MGDB_HARD_BREAKPOINT = (1 << 1),

    MGDB_SET_BREAKPOINT   = (1 << 2),
    MGDB_CLEAR_BREAKPOINT = (1 << 3)
};

// minigdbstub process call object
typedef struct
{
    char *regs;        // Pointer to register array
    size_t regsSize;   // Size of register array in bytes
    size_t regsCount;  // Total number of registers
    int signalNum;     // Signal that can be sent to GDB on certain operations
    mgdbOpts opts;     // Options bitfield
    int err;           // Return-error code
    void *usrData;     // Optional handle to opaque user data
} mgdbProcObj;

// ====================================================================================================================
// User stubs
// ====================================================================================================================
static void minigdbstubUsrPutchar(char data, void *usrData);
static char minigdbstubUsrGetchar(void *usrData);
static void minigdbstubUsrWriteMem(size_t addr, unsigned char data, void *usrData);
static unsigned char minigdbstubUsrReadMem(size_t addr, void *usrData);
static void minigdbstubUsrContinue(void *usrData);
static void minigdbstubUsrStep(void *usrData);
static void minigdbstubUsrProcessBreakpoint(int type, size_t addr, void *usrData);
static void minigdbstubUsrKillSession(void *usrData);
// ====================================================================================================================

static void minigdbstubComputeChecksum(char *buffer, size_t len, char *outBuf)
{
    unsigned int checksum = 0;
    for (size_t i = 0; i < len; ++i)
    {
        checksum = (checksum + buffer[i]) % 256;
    }
    MGDB_HEX_ENCODE_ASCII(checksum, 3, outBuf);

    // If the value is single digit
    if (outBuf[1] == 0)
    {
        outBuf[1] = outBuf[0];
        outBuf[0] = '0';
    }
}

static void minigdbstubSend(const char *data, mgdbProcObj *mgdbObj)
{
    size_t len = strlen(data);
    if (mgdbObj->opts.o_enableLogging)
    {
        MGDB_LOG_TRACE(MGDB_SEND " : packet = %s\n", data);
    }
    for (size_t i = 0; i < len; ++i)
    {
        minigdbstubUsrPutchar(data[i], mgdbObj->usrData);
    }
}

static void minigdbstubRecv(mgdbProcObj *mgdbObj, gdbPacket *gdbPkt)
{
    int currentOffset = 0;
    char c;
    while (1)
    {
        // Get the beginning of the packet data '$'
        while (1)
        {
            c = minigdbstubUsrGetchar(mgdbObj->usrData);
            if (c == '$')
            {
                break;
            }
        }

        // Read packet data until the end '#' - then read the remaining 2 checksum digits
        while (1)
        {
            c = minigdbstubUsrGetchar(mgdbObj->usrData);
            if (c == '#')
            {
                gdbPkt->checksum[0] = minigdbstubUsrGetchar(mgdbObj->usrData);
                gdbPkt->checksum[1] = minigdbstubUsrGetchar(mgdbObj->usrData);
                gdbPkt->checksum[2] = 0;
                MGDB_CHECK_RET(insertDynCharBuffer(&gdbPkt->pktData, 0), mgdbObj);
                break;
            }
            MGDB_CHECK_RET(insertDynCharBuffer(&gdbPkt->pktData, c), mgdbObj);
            ++currentOffset;
        }

        // Compute checksum and compare with expected checksum
        // Request retransmission if checksum verification fails
        char actualChecksum[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        minigdbstubComputeChecksum(gdbPkt->pktData.buffer, currentOffset, actualChecksum);
        if (strcmp(gdbPkt->checksum, actualChecksum) != 0)
        {
            gdbPkt->pktData.used = 0;
            minigdbstubSend(MGDB_RESEND_PACKET, mgdbObj);
            continue;
        }

        gdbPkt->commandType = gdbPkt->pktData.buffer[0];
        if (mgdbObj->opts.o_enableLogging)
        {
            MGDB_LOG_TRACE(MGDB_RECV " : packet = $%s#%s\n", gdbPkt->pktData.buffer,
                           gdbPkt->checksum);
        }
        minigdbstubSend(MGDB_ACK_PACKET, mgdbObj);
        return;
    }
}

static void minigdbstubWriteRegs(mgdbProcObj *mgdbObj, gdbPacket *recvPkt)
{
    char tmpBuf[8];
    int decodedHex;
    for (size_t i = 0; i < recvPkt->pktData.size - 1; ++i)
    {
        tmpBuf[i % 2] = recvPkt->pktData.buffer[i];
        if ((i % 2) != 0)
        {
            MGDB_HEX_DECODE_ASCII(tmpBuf, decodedHex);
            mgdbObj->regs[i / 2] = (char)decodedHex;
        }
    }
}

static void minigdbstubWriteReg(mgdbProcObj *mgdbObj, gdbPacket *recvPkt)
{
    int index, valOffset = 0;
    size_t regWidth = mgdbObj->regsSize / mgdbObj->regsCount;
    for (int i = 0; recvPkt->pktData.buffer[i] != 0; ++i)
    {
        if (recvPkt->pktData.buffer[i] == '=')
        {
            recvPkt->pktData.buffer[i] = 0;
            ++valOffset;
            break;
        }
        ++valOffset;
    }
    MGDB_HEX_DECODE_ASCII(&recvPkt->pktData.buffer[1], index);

    mgdbProcObj tmpProcObj = *mgdbObj;
    tmpProcObj.regsCount   = 1;
    tmpProcObj.regs        = &mgdbObj->regs[index * regWidth];
    tmpProcObj.regsSize    = regWidth;

    gdbPacket tmpRecPkt      = *recvPkt;
    tmpRecPkt.pktData.buffer = &recvPkt->pktData.buffer[valOffset];

    minigdbstubWriteRegs(&tmpProcObj, &tmpRecPkt);
    if (tmpProcObj.err != MGDB_SUCCESS)
    {
        mgdbObj->err = tmpProcObj.err;
    }
}

static void minigdbstubSendRegs(mgdbProcObj *mgdbObj)
{
    DynCharBuffer sendPkt;
    MGDB_CHECK_RET(initDynCharBuffer(&sendPkt, 512), mgdbObj);
    MGDB_CHECK_RET(insertDynCharBuffer(&sendPkt, '$'), mgdbObj);

    for (size_t i = 0; i < mgdbObj->regsSize; ++i)
    {
        char itoaBuff[8];
        MGDB_HEX_ENCODE_ASCII((unsigned char)mgdbObj->regs[i], 3, itoaBuff);

        // Swap if single digit
        if (itoaBuff[1] == 0)
        {
            itoaBuff[1] = itoaBuff[0];
            itoaBuff[0] = '0';
        }

        MGDB_CHECK_RET(insertDynCharBuffer(&sendPkt, itoaBuff[0]), mgdbObj);
        MGDB_CHECK_RET(insertDynCharBuffer(&sendPkt, itoaBuff[1]), mgdbObj);
    }

    // Compute and append the checksum
    char checksum[8];
    size_t len = sendPkt.used - 1;
    minigdbstubComputeChecksum(&sendPkt.buffer[1], len, checksum);
    MGDB_CHECK_RET(insertDynCharBuffer(&sendPkt, '#'), mgdbObj);
    MGDB_CHECK_RET(insertDynCharBuffer(&sendPkt, checksum[0]), mgdbObj);
    MGDB_CHECK_RET(insertDynCharBuffer(&sendPkt, checksum[1]), mgdbObj);
    MGDB_CHECK_RET(insertDynCharBuffer(&sendPkt, 0), mgdbObj);

    minigdbstubSend((const char *)sendPkt.buffer, mgdbObj);
    freeDynCharBuffer(&sendPkt);
}

static void minigdbstubSendReg(mgdbProcObj *mgdbObj, gdbPacket *recvPkt)
{
    int index;
    size_t regWidth = mgdbObj->regsSize / mgdbObj->regsCount;
    MGDB_HEX_DECODE_ASCII(&recvPkt->pktData.buffer[2], index);

    mgdbProcObj tmpProcObj = *mgdbObj;
    tmpProcObj.regsCount   = 1;
    tmpProcObj.regs        = &mgdbObj->regs[index * regWidth];
    tmpProcObj.regsSize    = regWidth;

    minigdbstubSendRegs(&tmpProcObj);
    if (tmpProcObj.err != MGDB_SUCCESS)
    {
        mgdbObj->err = tmpProcObj.err;
    }
}

static void minigdbstubWriteMem(mgdbProcObj *mgdbObj, gdbPacket *recvPkt)
{
    size_t address, length;
    int lengthOffset = 0;
    int valOffset    = 0;

    // Grab the length offset
    for (int i = 0; recvPkt->pktData.buffer[i] != 0; ++i)
    {
        if ((recvPkt->pktData.buffer[i] == ',') || (recvPkt->pktData.buffer[i] == ';') ||
            (recvPkt->pktData.buffer[i] == ':'))
        {
            recvPkt->pktData.buffer[i] = 0;
            ++lengthOffset;
            valOffset = lengthOffset;
            break;
        }
        ++lengthOffset;
    }
    // Grab the data value offset
    for (int i = 0; recvPkt->pktData.buffer[i] != 0; ++i)
    {
        if ((recvPkt->pktData.buffer[i] == ',') || (recvPkt->pktData.buffer[i] == ';') ||
            (recvPkt->pktData.buffer[i] == ':'))
        {
            recvPkt->pktData.buffer[i] = 0;
            ++valOffset;
            break;
        }
        ++valOffset;
    }
    MGDB_HEX_DECODE_ASCII(&recvPkt->pktData.buffer[1], address);
    MGDB_HEX_DECODE_ASCII(&recvPkt->pktData.buffer[lengthOffset], length);

    // Call user write memory handler
    for (size_t i = 0; i < length; ++i)
    {
        int decodedVal;
        char atoiBuf[8];
        atoiBuf[0] = recvPkt->pktData.buffer[valOffset + (i * 2)];
        atoiBuf[1] = recvPkt->pktData.buffer[valOffset + (i * 2) + 1];
        MGDB_HEX_DECODE_ASCII(atoiBuf, decodedVal);
        minigdbstubUsrWriteMem(address + i, (unsigned char)decodedVal, mgdbObj->usrData);
    }

    // Send ACK to GDB
    minigdbstubSend(MGDB_ACK_PACKET, mgdbObj);
}

static void minigdbstubReadMem(mgdbProcObj *mgdbObj, gdbPacket *recvPkt)
{
    size_t address, length;
    int valOffset = 0;
    for (int i = 0; recvPkt->pktData.buffer[i] != 0; ++i)
    {
        if ((recvPkt->pktData.buffer[i] == ',') || (recvPkt->pktData.buffer[i] == ';') ||
            (recvPkt->pktData.buffer[i] == ':'))
        {
            recvPkt->pktData.buffer[i] = 0;
            ++valOffset;
            break;
        }
        ++valOffset;
    }
    MGDB_HEX_DECODE_ASCII(&recvPkt->pktData.buffer[1], address);
    MGDB_HEX_DECODE_ASCII(&recvPkt->pktData.buffer[valOffset], length);

    // Alloc a packet w/ the requested data to send as a response to GDB
    DynCharBuffer memBuf;
    MGDB_CHECK_RET(initDynCharBuffer(&memBuf, length), mgdbObj);
    MGDB_CHECK_RET(insertDynCharBuffer(&memBuf, '$'), mgdbObj);

    // Call user read memory handler
    for (size_t i = 0; i < length; ++i)
    {
        char itoaBuff[8];
        unsigned char c = minigdbstubUsrReadMem(address + i, mgdbObj->usrData);
        MGDB_HEX_ENCODE_ASCII(c, 3, itoaBuff);

        // Swap if single digit
        if (itoaBuff[1] == 0)
        {
            itoaBuff[1] = itoaBuff[0];
            itoaBuff[0] = '0';
        }

        MGDB_CHECK_RET(insertDynCharBuffer(&memBuf, itoaBuff[0]), mgdbObj);
        MGDB_CHECK_RET(insertDynCharBuffer(&memBuf, itoaBuff[1]), mgdbObj);
    }

    // Compute and append the checksum
    char checksum[8];
    size_t len = memBuf.used - 1;
    minigdbstubComputeChecksum(&memBuf.buffer[1], len, checksum);
    MGDB_CHECK_RET(insertDynCharBuffer(&memBuf, '#'), mgdbObj);
    MGDB_CHECK_RET(insertDynCharBuffer(&memBuf, checksum[0]), mgdbObj);
    MGDB_CHECK_RET(insertDynCharBuffer(&memBuf, checksum[1]), mgdbObj);
    MGDB_CHECK_RET(insertDynCharBuffer(&memBuf, 0), mgdbObj);

    minigdbstubSend((const char *)memBuf.buffer, mgdbObj);
    freeDynCharBuffer(&memBuf);
}

static void minigdbstubSendSignal(mgdbProcObj *mgdbObj)
{
    DynCharBuffer sendPkt;
    MGDB_CHECK_RET(initDynCharBuffer(&sendPkt, 32), mgdbObj);
    MGDB_CHECK_RET(insertDynCharBuffer(&sendPkt, '$'), mgdbObj);
    MGDB_CHECK_RET(insertDynCharBuffer(&sendPkt, 'S'), mgdbObj);

    // Convert signal num to hex char array
    char itoaBuff[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    MGDB_DEC_ENCODE_ASCII(mgdbObj->signalNum, 8, itoaBuff);

    // Swap if single digit
    if (itoaBuff[1] == 0)
    {
        itoaBuff[1] = itoaBuff[0];
        itoaBuff[0] = '0';
    }

    int bufferPtr = 0;
    while (itoaBuff[bufferPtr] != 0)
    {
        MGDB_CHECK_RET(insertDynCharBuffer(&sendPkt, itoaBuff[bufferPtr]), mgdbObj);
        ++bufferPtr;
    }
    MGDB_CHECK_RET(insertDynCharBuffer(&sendPkt, '#'), mgdbObj);

    // Add the two checksum hex chars
    char checksumHex[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    minigdbstubComputeChecksum(&sendPkt.buffer[1], bufferPtr + 1, checksumHex);
    MGDB_CHECK_RET(insertDynCharBuffer(&sendPkt, checksumHex[0]), mgdbObj);
    MGDB_CHECK_RET(insertDynCharBuffer(&sendPkt, checksumHex[1]), mgdbObj);
    MGDB_CHECK_RET(insertDynCharBuffer(&sendPkt, 0), mgdbObj);

    minigdbstubSend((const char *)sendPkt.buffer, mgdbObj);
    freeDynCharBuffer(&sendPkt);
}

static void minigdbstubProcessBreakpoint(mgdbProcObj *mgdbObj, gdbPacket *recvPkt, int type)
{
    int offset = 0;
    size_t address;
    for (int i = 0; recvPkt->pktData.buffer[i] != 0; ++i)
    {
        if ((recvPkt->pktData.buffer[i] == ',') || (recvPkt->pktData.buffer[i] == ';') ||
            (recvPkt->pktData.buffer[i] == ':'))
        {
            if (offset > 0)
            {
                recvPkt->pktData.buffer[i] = 0;
                break;
            }
            offset = i + 1;
        }
    }
    MGDB_HEX_DECODE_ASCII(&recvPkt->pktData.buffer[offset], address);

    switch (recvPkt->pktData.buffer[2])
    {
        case '0':  // Software breakpoint
            type |= MGDB_SOFT_BREAKPOINT;
            break;
        case '1':  // Hardware breakpoint
            type |= MGDB_HARD_BREAKPOINT;
            break;
        default:  // Other breakpoint/watchpoint type (unsupported)
            break;
    }
    minigdbstubUsrProcessBreakpoint(type, address, mgdbObj->usrData);

    // Send ACK + OK to GDB
    minigdbstubSend(MGDB_ACK_PACKET, mgdbObj);
    minigdbstubSend(MGDB_OK_PACKET, mgdbObj);
}

// Main gdb stub process call
static void minigdbstubProcess(mgdbProcObj *mgdbObj)
{
    if (mgdbObj->opts.o_signalOnEntry)
    {
        minigdbstubSendSignal(mgdbObj);
    }
    // Poll and reply to packets from GDB until exit-related command
    while (1)
    {
        gdbPacket recvPkt;
        MGDB_CHECK_RET(initDynCharBuffer(&recvPkt.pktData, MGDB_PKT_SIZE), mgdbObj);
        minigdbstubRecv(mgdbObj, &recvPkt);

        switch (recvPkt.commandType)
        {
            case 'g':
            {  // Read registers
                minigdbstubSendRegs(mgdbObj);
                break;
            }
            case 'G':
            {  // Write registers
                minigdbstubWriteRegs(mgdbObj, &recvPkt);
                break;
            }
            case 'p':
            {  // Read one register
                minigdbstubSendReg(mgdbObj, &recvPkt);
                break;
            }
            case 'P':
            {  // Write one register
                minigdbstubWriteReg(mgdbObj, &recvPkt);
                break;
            }
            case 'm':
            {  // Read mem
                minigdbstubReadMem(mgdbObj, &recvPkt);
                break;
            }
            case 'M':
            {  // Write mem
                minigdbstubWriteMem(mgdbObj, &recvPkt);
                break;
            }
            case 'c':
            {  // Continue
                minigdbstubUsrContinue(mgdbObj->usrData);
                return;
            }
            case 's':
            {  // Step
                minigdbstubUsrStep(mgdbObj->usrData);
                return;
            }
            case 'Z':
            {  // Place breakpoint
                minigdbstubProcessBreakpoint(mgdbObj, &recvPkt, MGDB_SET_BREAKPOINT);
                break;
            }
            case 'z':
            {  // Remove breakpoint
                minigdbstubProcessBreakpoint(mgdbObj, &recvPkt, MGDB_CLEAR_BREAKPOINT);
                break;
            }
            case 'k':
            {  // Kill session
                minigdbstubUsrKillSession(mgdbObj->usrData);
                return;
            }
            case '?':
            {  // Indicate reason why target halted
                minigdbstubSendSignal(mgdbObj);
                break;
                ;
            }
            default:
            {  // Command unsupported
                minigdbstubSend(MGDB_EMPTY_PACKET, mgdbObj);
                break;
            }
        }
        // Cleanup packet mem
        freeDynCharBuffer(&recvPkt.pktData);
    }
}
