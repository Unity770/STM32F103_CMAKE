
/*
说明
    此驱动不做硬件初始化，请在使用前自行初始化硬件。
    其驱动需要配合环形缓冲区使用。


*/
#ifndef __FY_UART_H
#define __FY_UART_H

#include "fy_ringBuffer.h"
#include "stm32f1xx_hal_uart.h"
#include "usart.h"

typedef struct fy_uart fy_uart_t;

typedef struct fy_uart {
    //成员
    UART_HandleTypeDef *huart;
    ringBuffer_t* rx_rb;
    ringBuffer_t* tx_rb;
    uint8_t rx_temp_byte;//中断接收暂存的数据
    size_t tx_active_len;
    //方法
    uint16_t (*uartTx)(struct fy_uart *uart, const uint8_t *data, size_t len);
    uint16_t (*uartRx)(struct fy_uart *uart, uint8_t *out, size_t len);
    void (*uartClear_txBuffer)(struct fy_uart *uart);
    void (*uartClear_rxBuffer)(struct fy_uart *uart);
} fy_uart_t;

int32_t fy_uart_init(fy_uart_t *uart, UART_HandleTypeDef *huart, ringBuffer_t* rx_rb,ringBuffer_t* tx_rb);
void fy_uart_tx(fy_uart_t *fy_uart, const uint8_t *data, size_t len);
uint16_t fy_uart_rx(fy_uart_t *fy_uart, uint8_t *out, size_t len);
#endif