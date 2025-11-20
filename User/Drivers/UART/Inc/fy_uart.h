#ifndef __FY_UART_H
#define __FY_UART_H

#include "fy_ringBuffer.h"
#include "stm32f1xx_hal_uart.h"
#include "usart.h"

typedef struct fy_uart {
    UART_HandleTypeDef *huart;
    ringBuffer_t rx_rb;
    ringBuffer_t tx_rb;
    uint8_t rx_temp_byte;
    size_t tx_active_len;
} fy_uart_t;

void fy_uart_init(fy_uart_t *uart, UART_HandleTypeDef *huart);
void fy_uart_tx(fy_uart_t *fy_uart, const uint8_t *data, size_t len);
uint16_t fy_uart_rx(fy_uart_t *fy_uart, uint8_t *out, size_t len);
#endif