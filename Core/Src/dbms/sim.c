//
//  Simulation support (TCP sockets)
//
#ifdef SIM

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>

#ifdef __clang__
// some compilers in sim builds may miss stdint
typedef int int32_t;
typedef unsigned int uint32_t;
#endif

#include "sim.h"   // should provide __SimCtx, HAL typedefs, etc.

// ----------------------------------------------------
//            Config: TCP endpoints
// ----------------------------------------------------
#define SIM_HOST_LOOPBACK  ((uint32_t)0x7F000001u) // 127.0.0.1
#define SIM_PORT_UART      4001
#define SIM_PORT_CAN       4002

// ----------------------------------------------------
//            Global sim context
// ----------------------------------------------------
static __SimCtx __sim_ctx;

// ----------------------------------------------------
//            TCP connect helper
// ----------------------------------------------------
static int __connect_tcp_loopback(uint16_t port)
{
    int fd;
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port   = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1

    for (;;)
    {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            perror("IPC: socket()");
            sleep(1);
            continue;
        }
        if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) == 0) {
            // optional: set TCP_NODELAY if needed
            return fd;
        }
        close(fd);
        // Retry until server comes up
        usleep(200 * 1000);
    }
}

// ----------------------------------------------------
//            Sim enter/exit & raw send
// ----------------------------------------------------
void __SimEnter(char* ipc_path_can, char* ipc_path_uart)
{
    (void)ipc_path_can;  // unused (ports are fixed)
    (void)ipc_path_uart; // unused

    __sim_ctx.ipc_fd_can  = __connect_tcp_loopback(SIM_PORT_CAN);
    __sim_ctx.ipc_fd_uart = __connect_tcp_loopback(SIM_PORT_UART);
    __sim_ctx.can_started = false;
}

void __SimExit()
{
    if (__sim_ctx.ipc_fd_can  >= 0) close(__sim_ctx.ipc_fd_can);
    if (__sim_ctx.ipc_fd_uart >= 0) close(__sim_ctx.ipc_fd_uart);
}

int __SimIpcSend(int fd, const unsigned char* data, int size)
{
    int total_written = 0;
    while (total_written < size) {
        int written = (int)write(fd, data + total_written, (size_t)(size - total_written));
        if (written < 0) {
            if (errno == EINTR) continue;
            perror("IPC: write");
            return -1;
        }
        total_written += written;
    }
    return total_written;
}

// ----------------------------------------------------
//            CAN RX accumulation & queue
//            (NO Dbms calls here)
// ----------------------------------------------------
typedef struct {
    uint32_t id_be;  // 4B big-endian ID (as on the wire)
    uint8_t  data[8];
} SimCanFrame;

// stream accumulator
static uint8_t s_can_rx_accum[4096];
static size_t  s_can_rx_len = 0;

// simple ring queue
#define CANQ_CAP 256
static SimCanFrame s_can_q[CANQ_CAP];
static unsigned s_can_q_head = 0; // next write
static unsigned s_can_q_tail = 0; // next read

static inline int __canq_push(const SimCanFrame* f)
{
    unsigned nhead = (s_can_q_head + 1u) % CANQ_CAP;
    if (nhead == s_can_q_tail) {
        // overflow policy: drop oldest
        s_can_q_tail = (s_can_q_tail + 1u) % CANQ_CAP;
    }
    s_can_q[s_can_q_head] = *f;
    s_can_q_head = nhead;
    return 1;
}

static inline int __canq_pop(SimCanFrame* f)
{
    if (s_can_q_tail == s_can_q_head) return 0;
    *f = s_can_q[s_can_q_tail];
    s_can_q_tail = (s_can_q_tail + 1u) % CANQ_CAP;
    return 1;
}

void __SimCanPoll(void)
{
    if (__sim_ctx.ipc_fd_can < 0) return;

    // 1) Non-blocking read from socket
    for (;;)
    {
        uint8_t tmp[1024];
        ssize_t n = recv(__sim_ctx.ipc_fd_can, tmp, sizeof(tmp), MSG_DONTWAIT);
        if (n <= 0) break;

        if (s_can_rx_len + (size_t)n > sizeof(s_can_rx_accum)) {
            // overflow: reset accumulator
            s_can_rx_len = 0;
        }
        memcpy(s_can_rx_accum + s_can_rx_len, tmp, (size_t)n);
        s_can_rx_len += (size_t)n;
    }

    // 2) Extract complete 12B frames: [ID(4, BE)] [DATA(8)]
    while (s_can_rx_len >= 12)
    {
        SimCanFrame f;
        f.id_be = ((uint32_t)s_can_rx_accum[0] << 24) |
                  ((uint32_t)s_can_rx_accum[1] << 16) |
                  ((uint32_t)s_can_rx_accum[2] <<  8) |
                  ((uint32_t)s_can_rx_accum[3] <<  0);
        memcpy(f.data, s_can_rx_accum + 4, 8);

        __canq_push(&f);

        size_t remain = s_can_rx_len - 12;
        memmove(s_can_rx_accum, s_can_rx_accum + 12, remain);
        s_can_rx_len = remain;
    }
}

int __SimCanPop(SimCanFrame* out)
{
    return __canq_pop(out);
}

// ----------------------------------------------------
//            HAL Spoof (TX -> sockets)
// ----------------------------------------------------
HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *hcan, const CAN_FilterTypeDef *sFilterConfig)
{
    return HAL_OK;
}

HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan)
{
    __sim_ctx.can_started = true;
    return HAL_OK;
}

uint32_t HAL_CAN_GetTxMailboxesFreeLevel(const CAN_HandleTypeDef *hcan)
{
    return 0; // not modeled
}

HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan, const CAN_TxHeaderTypeDef *pHeader,
                                       const uint8_t aData[], uint32_t *pTxMailbox)
{
    if (!__sim_ctx.can_started) return HAL_BUSY;

    uint8_t framebuf[12];
    uint32_t id = pHeader->StdId; // 11-bit used; placed into low bits of 32-bit field
    framebuf[0] = (uint8_t)((id >> 24) & 0xFF);
    framebuf[1] = (uint8_t)((id >> 16) & 0xFF);
    framebuf[2] = (uint8_t)((id >>  8) & 0xFF);
    framebuf[3] = (uint8_t)( id        & 0xFF);
    memcpy(framebuf + 4, aData, 8);

    __SimIpcSend(__sim_ctx.ipc_fd_can, framebuf, 12);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc) { return HAL_OK; }
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t Timeout) { return HAL_OK; }

static int __sim_adc_counter = 0;
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc)
{
    __sim_adc_counter = (__sim_adc_counter + 1) % 1000;
    return (uint32_t)__sim_adc_counter;
}

HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *hadc) { return HAL_OK; }

void HAL_Delay(uint32_t Delay) { usleep(Delay * 1000); }

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    (void)huart; (void)Timeout;
    __SimIpcSend(__sim_ctx.ipc_fd_uart, pData, Size);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size, uint32_t Timeout)
// HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    (void)huart;
    if (__sim_ctx.ipc_fd_uart < 0) return HAL_ERROR;

    uint8_t *dst = pData;
    size_t remaining = Size;

    // Compute deadline (ms)
    struct timeval start, now;
    gettimeofday(&start, NULL);
    uint64_t start_ms = (uint64_t)start.tv_sec * 1000u + (uint64_t)(start.tv_usec / 1000u);

    while (remaining > 0) {
        // How much time left?
        gettimeofday(&now, NULL);
        uint64_t now_ms = (uint64_t)now.tv_sec * 1000u + (uint64_t)(now.tv_usec / 1000u);
        uint64_t elapsed = now_ms - start_ms;

        // If Timeout == 0 -> immediate poll
        int64_t left_ms = (Timeout == 0xFFFFFFFFu) ? -1 : ((int64_t)Timeout - (int64_t)elapsed);
        if (Timeout != 0xFFFFFFFFu && left_ms <= 0) {
            return HAL_TIMEOUT;
        }

        // Wait for readability
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(__sim_ctx.ipc_fd_uart, &rfds);

        struct timeval tv;
        if (left_ms < 0) {
            // block indefinitely (HAL_MAX_DELAY style)
            tv.tv_sec = 5;  // safety wakeup; we’ll loop again
            tv.tv_usec = 0;
        } else {
            tv.tv_sec  = (time_t)(left_ms / 1000);
            tv.tv_usec = (suseconds_t)((left_ms % 1000) * 1000);
        }

        int rv = select(__sim_ctx.ipc_fd_uart + 1, &rfds, NULL, NULL, &tv);
        if (rv == 0) {
            // timed out for this select window; check deadline in next loop
            if (Timeout != 0xFFFFFFFFu) {
                gettimeofday(&now, NULL);
                now_ms = (uint64_t)now.tv_sec * 1000u + (uint64_t)(now.tv_usec / 1000u);
                if (now_ms - start_ms >= Timeout) return HAL_TIMEOUT;
            }
            continue;
        }
        if (rv < 0) {
            if (errno == EINTR) continue;
            return HAL_ERROR;
        }

        // Read whatever is available
        ssize_t n = read(__sim_ctx.ipc_fd_uart, dst, remaining);
        if (n < 0) {
            if (errno == EINTR) continue;
            return HAL_ERROR;
        }
        if (n == 0) {
            // peer closed
            return HAL_ERROR;
        }

        dst += (size_t)n;
        remaining -= (size_t)n;
    }

    return HAL_OK;
}

void HAL_TIM_Base_Start(TIM_HandleTypeDef *huart) {}

HAL_StatusTypeDef HAL_GPIO_WritePin(GPIO_TypeDef* x, int p, int a) { return HAL_OK; }

int HAL_GetTick()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int)((uint64_t)tv.tv_sec * 1000 + (tv.tv_usec / 1000));
}

int HAL_I2C_Master_Transmit(I2C_HandleTypeDef* _, int addr, char* buf, int size, int timeout)
{
    (void)_; (void)timeout;
    char filename[64];
    snprintf(filename, sizeof(filename), "eeprom_%d.txt", addr);
    FILE* f = fopen(filename, "wb");
    if (!f) return -1;
    size_t written = fwrite(buf, 1, (size_t)size, f);
    fclose(f);
    return (written == (size_t)size) ? 0 : -1;
}

int HAL_I2C_Master_Receive(I2C_HandleTypeDef* _, int addr, char* buf, int size, int timeout)
{
    (void)_; (void)timeout;
    char filename[64];
    snprintf(filename, sizeof(filename), "eeprom_%d.txt", addr);
    FILE* f = fopen(filename, "rb");
    if (!f) return -1;
    size_t rd = fread(buf, 1, (size_t)size, f);
    fclose(f);
    if (rd < (size_t)size) memset(buf + rd, 0, (size_t)size - rd);
    return (rd > 0) ? 0 : -1;
}

int HAL_CAN_ActivateNotification(CAN_HandleTypeDef* j, int k) { return 0; }

#endif // SIM
