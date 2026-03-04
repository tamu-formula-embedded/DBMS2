#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define N_GROUPS_PER_SIDE 1

// External dependencies
typedef struct {
    struct { int uart; } hw;
} DbmsCtx;
uint16_t CalcCrc(uint8_t*, int);
int HAL_UART_Transmit(int, char*, int, int);
#define N_MONITORS 2
#define N_GROUPS_PER_SIDE 13


/**
 * IMPORTANT: make sure struct has no padding...
 * ... which is any struct that goes over the wire.
 */
#define PACKED __attribute__((packed))

/**
 * Swapping endian-ness is super important when using 
 * on-the-wire structs. We need an endian-swap (eswap)
 * function for 16 and 32 bit. 
 * 
 * For runtime variables, we use the __builtin_bswap* 
 * functions, as these compile to the rev* instructions.
 * But for constants, we use the traditional method
 * so it compiles in a constexpr context. We can detect 
 * if something is constant using __builtin_constant_p.
 */
// #define ESWAP16_CONST(x) \
//     (uint16_t)((((uint16_t)(x) & 0x00FFu) << 8) | \
//                (((uint16_t)(x) & 0xFF00u) >> 8))
// #define ESWAP32_CONST(x) \
//     ((((uint32_t)(x) & 0x000000FFu) << 24) | \
//      (((uint32_t)(x) & 0x0000FF00u) << 8)  | \
//      (((uint32_t)(x) & 0x00FF0000u) >> 8)  | \
//      (((uint32_t)(x) & 0xFF000000u) >> 24))

// #define ESWAP16(x)  (__builtin_constant_p(x) ? ESWAP16_CONST(x) : __builtin_bswap16(x))
// #define ESWAP32(x)  (__builtin_constant_p(x) ? ESWAP32_CONST(x) : __builtin_bswap32(x))

#define ESWAP16(x) __builtin_bswap16((uint16_t)(x))
#define ESWAP32(x) __builtin_bswap32((uint32_t)(x))

#define STACK_SEND_TIMEOUT  10
#define STACK_RECV_TIMEOUT  10

/**
 * Important data
 */
#define MAX_TX_DATA 8
#define MAX_RX_DATA 128

#define REQ_SINGLE_DEV_READ         0   // 0x08
#define REQ_SINGLE_DEV_WRITE        1   // 0x09
#define REQ_STACK_READ              2   // 0x0A
#define REQ_STACK_WRITE             3   // 0x0B
#define REQ_BROADCAST_READ          4   // 0x0C
#define REQ_BROADCAST_WRITE         5   // 0x0D
#define REQ_BROADCAST_WRITE_REV     6   // 0x0E


typedef struct PACKED _TxStackFrame 
{
    uint8_t     __cmd   : 1;                /* must be set to 1 */
    uint8_t     reqtype : 3;                /* one of REQ_* */
    uint8_t     __res   : 1;                /* reserved bit */
    uint8_t     len     : 3;                
    uint8_t     devaddr;
    uint16_t    regaddr;
    uint8_t     data[MAX_TX_DATA];
    uint16_t    __crc;                      /* extra buffer for CRC if all data bytes
                                                are used. So the real location of CRC
                                                is at f->data + f->len */
} TxStackFrame;

typedef struct PACKED _RxStackFrame
{
    uint8_t     __cmd   : 1;                /* must be set to 0 */
    uint8_t     len     : 7;                
    uint8_t     devaddr;
    uint16_t    regaddr;
    uint8_t     data[MAX_RX_DATA];
    uint16_t    __crc;                      /* extra buffer for CRC if all data bytes
                                                are used. So the real location of CRC
                                                is at f->data + f->len */
} RxStackFrame;

/**
 * Helpful macros
 */

#define CRC(F)          *((uint16_t*)((F).data + (F).len))

#define CALC_CRC(F)     CalcCrc((uint8_t*)(&F), (4 + (F).len))

#define FRAME_LEN(F)    ((F).len + 6)

#define MAKE_TX_FRAME(REQTYPE, DEVADDR, REGADDR, ...)       \
((TxStackFrame){                                            \
    .__cmd   = 1,                                           \
    .reqtype = (REQTYPE),                                   \
    .__res   = 0,                                           \
    .len     = sizeof((uint8_t[]){ __VA_ARGS__ }),          \
    .devaddr = (DEVADDR),                                   \
    .regaddr = ESWAP16(REGADDR),                            \
    .data    = { __VA_ARGS__ },                             \
    .__crc   = 0                                            \
})
// example: 
static uint8_t frame1[] = {0xB2, 0x00, 0x0E, 0x09, 0x2D, 0x09, 0x00, 0x00};
// becomes:
static TxStackFrame frame2 = MAKE_TX_FRAME(REQ_STACK_WRITE, 0, 0x0E09, /*data=*/ 0x2D, 0x09 );


/**
 * We need a new send function now
 */
int SendStackFrame(DbmsCtx* ctx, TxStackFrame frame)
{
    CRC(frame) = ESWAP16(CALC_CRC(frame));
    return HAL_UART_Transmit(ctx->hw.uart, (uint8_t*)&frame, FRAME_LEN(frame), STACK_SEND_TIMEOUT);
}

/**
 * And we can use this like
 */
int StackSetupGpio(DbmsCtx* ctx)
{
    SendStackFrame(ctx, MAKE_TX_FRAME(REQ_STACK_WRITE, 0, 0x0E09, /*data=*/ 0x2D, 0x09));

    SendStackFrame(ctx, MAKE_TX_FRAME(REQ_STACK_WRITE, 0x03, 0x0A01));
}


/**
 * For recv we can abstract away a lot of the logic
 */



void StackUpdateAllVoltReadings(DbmsCtx* ctx)
{
    int regaddr = (0x68 + 2 * (16 - N_GROUPS_PER_SIDE) << 8) + (N_GROUPS_PER_SIDE * 2 - 1);
    SendStackFrame(ctx, MAKE_TX_FRAME(REQ_STACK_READ, 0x05, regaddr));

    RxStackFrame rxframe = {0};
    
}