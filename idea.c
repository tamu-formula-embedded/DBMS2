#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define N_GROUPS_PER_SIDE 1

typedef struct DbmsCtx DbmsCtx;



/**
 * IMPORTANT: make sure struct has no padding...
 * ... which is any struct that goes over the wire.
 */
#define PACKED __attribute__((packed))

/**
 * Swapping endian-ness is super important when using 
 * on-the-wire structs. We need an endian-swap (eswap)
 * function for 16 and 32 bit. There are actually 
 * instructions that do that, which are exposed with
 * "compiler intrinsics" __builtin_*. 
 */
#define eswap16(X) __builtin_bswap16(X)
#define eswap32(X) __builtin_bswap32(X)

/**
 * A base struct to hold the common header info
 */
typedef struct PACKED _FrameHdr 
{
    uint8_t init;
    uint8_t devaddr;
    uint16_t regaddr;
} FrameHdr;

/**
 * Define a "constructor" for the header
 */
#define MAKE_HDR(INIT, DA, RA)  (FrameHdr){ INIT, DA, eswap16(RA) }

/**
 * Simple frame with no data
 */
typedef struct PACKED _CmdFrame 
{
    FrameHdr h;
    uint16_t crc;
} CmdFrame;

/**
 * Define a "constructor" for this frame
 */
#define MAKE_CMD(INIT, DA, RA) (CmdFrame){ MAKE_HDR(INIT, DA, RA), 0 }

/**
 * Frame for voltage response
 */
typedef struct PACKED _VoltFrame
{
    FrameHdr h;
    uint16_t voltages[N_GROUPS_PER_SIDE];       // data for the voltages
    uint16_t crc;
} VoltFrame;


/**
 * NOTE: defining frames is not much different
 */
static uint8_t frame1[] = {0xD0, 0x03, 0x0D, 0x06, 0x00, 0x00};     // bad
static CmdFrame frame2 = {{0xD0, 0x03, eswap16(0x0D06)}, 0};        // not much better
static CmdFrame frame3 = MAKE_CMD(0xD0, 0x03, 0x0D06);              // good

/**
 * Send frame is basically the same, but takes a void* frame so we can do
 * 
 *  CmdFrame frame = ...;
 *  SendStackFrameSetCrc(ctx, &frame, sizeof(frame);
 */
int SendStackFrameSetCrc(DbmsCtx* ctx, void* frame, size_t len);

/**
 * And we can do some shortcuts for sending frames 
 * At least the command frame since it is sent a lot...
 */
int SendCmdFrame(DbmsCtx* ctx, CmdFrame frame)
{
    return SendStackFrameSetCrc(ctx, &frame, sizeof(CmdFrame));
}

/**
 * ...which we can call like so
 */
int Example1(DbmsCtx* ctx)
{
    SendCmdFrame(ctx, MAKE_CMD(0xD0, 0x03, 0x0D06)); // allows us to send without 
}

/**
 * We can also give names to these actions / registers even
 */


/**
 * You guys should know the following standard library functions
 * from old school Unix. These use the legacy long=32 semantics.
 * These were made for arpanet and are still around, you 
 * may have used them with sockets. 
 * 
 * These semantics handle variable host byte order. 
 *  Host = your CPU, can be big or little endian
 *  Network = the wire, agreed to be big endian
 * 
 * So if your host is big endian, the function does nothing,
 * otherwise it bswaps. Good for portability, but obviously
 * doesn't matter on STM32.
 */
short htons(short); // host-to-network-short
                    // 16-bit little endian -> big endian
long htonl(long);   // host-to-network-long 
                    // 32-bit little endian -> big endian
short ntohs(short); // network-to-host-short
                    // 16-bit big endian -> little endian
long ntohl(long);   // network-to-host-long
                    // 32-bit big endian -> little endian