#include "userMain.h"

uint8_t uart1_rx_buffer[256];
uint8_t uart1_tx_buffer[256];
ringBuffer_t uart1_rx_rb;
ringBuffer_t uart1_tx_rb;

fy_uart_t uart1;

void uart1_init(void)
{
    ringBuffer_init(&uart1_rx_rb, uart1_rx_buffer, sizeof(uart1_rx_buffer));
    ringBuffer_init(&uart1_tx_rb, uart1_tx_buffer, sizeof(uart1_tx_buffer));

    fy_uart_init(&uart1, &huart1, &uart1_rx_rb, &uart1_tx_rb);
}

void user_main(void)
{
    /* Example usage of fy_uart and ring buffer can be placed here */
    uint32_t start_tick = HAL_GetTick();
    uart1_init();
    while (1)
    {
        HAL_GetTick();
        if (HAL_GetTick() - start_tick >= 100)
        {
            start_tick = HAL_GetTick();
            // 每隔1秒发送一次数据
            const char *msg = "Hello from UART1!\r\n";
            uart1.uartTx(&uart1, (const uint8_t *)msg, strlen(msg));
        }
    }
    
}