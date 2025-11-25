#include "fy_uart.h"

/* Private variables ---------------------------------------------------------*/
/* small registry to support multiple fy_uart_t instances
   mapped by their HAL `UART_HandleTypeDef *`. */
static fy_uart_t *uart_registry[8] = {0};
/* Private functions ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

static int register_uart_instance(fy_uart_t *uart)
{
    for (size_t i = 0; i < sizeof(uart_registry)/sizeof(uart_registry[0]); ++i) {
        if (uart_registry[i] == NULL) {
            uart_registry[i] = uart;
            return 0;
        }
    }
    return -1;
}

/*static void unregister_uart_instance(fy_uart_t *uart)
{
    for (size_t i = 0; i < sizeof(uart_registry)/sizeof(uart_registry[0]); ++i) {
        if (uart_registry[i] == uart) {
            uart_registry[i] = NULL;
            return;
        }
    }
}*/

static fy_uart_t *find_uart_by_huart(UART_HandleTypeDef *huart)
{
    for (size_t i = 0; i < sizeof(uart_registry)/sizeof(uart_registry[0]); ++i) {
        if (uart_registry[i] != NULL && uart_registry[i]->huart == huart) {
            return uart_registry[i];
        }
    }
    return NULL;
}

static void uart_try_transmit(fy_uart_t *uart)
{
    ringBuffer_t *rb = uart->tx_rb;
    
    /* Check buffer count */
    size_t len_to_send = rb->used(rb);

    /* Calculate contiguous data length from tail */

    if (len_to_send > 0) {
        /* Fix: Ensure we don't read past the end of the physical buffer */
        size_t contiguous_len = rb->size - rb->tail;
        if (len_to_send > contiguous_len) {
            len_to_send = contiguous_len;
        }

        uart->tx_active_len = len_to_send;
        /* Call HAL_UART_Transmit_DMA */
        const uint8_t *rb_tail = rb->rb_tail(rb);
        HAL_UART_Transmit_DMA(uart->huart, rb_tail, len_to_send);
    }
}
/* HAL Callbacks -------------------------------------------------------------*/

static void fy_uart_tx_cplt_callback(UART_HandleTypeDef *huart)
{
    /* Look up fy_uart_t instance for this HAL handle */
    fy_uart_t *uart = find_uart_by_huart(huart);
    if (uart != NULL) {
        ringBuffer_t *rb = uart->tx_rb;

        /* Update tail for the remaining part */
        size_t half = uart->tx_active_len % 2 == 0?uart->tx_active_len/2:(uart->tx_active_len/2)+1;//dma奇数传输像上计算
        size_t remaining = uart->tx_active_len - half;
        
        rb->read(rb, NULL, remaining);
        uart->tx_active_len = 0;

        /* Check buffer count and transmit next chunk if available */
        uart_try_transmit(uart);
    }
}

static void fy_uart_tx_half_cplt_callback(UART_HandleTypeDef *huart)
{
    fy_uart_t *uart = find_uart_by_huart(huart);
    if (uart != NULL) {
        ringBuffer_t *rb = uart->tx_rb;

        /* Update tail for the first half */
        size_t half = uart->tx_active_len % 2 == 0?uart->tx_active_len/2:(uart->tx_active_len/2)+1;//dma奇数传输像上计算
        if (half > 0) {
            rb->read(rb, NULL, half);
        }
    }
}

static void fy_uart_rx_cplt_callback(UART_HandleTypeDef *huart)
{
    fy_uart_t *uart = find_uart_by_huart(huart);
    if (uart != NULL) {
        /* Push received byte to ringbuffer */
        uart->rx_rb->write(uart->rx_rb, &uart->rx_temp_byte, 1);

        /* Restart reception */
        HAL_UART_Receive_IT(huart, &uart->rx_temp_byte, 1);
    }
}
uint16_t fy_uart_tx(fy_uart_t *uart, const uint8_t *data, size_t len)
{
    if (uart == NULL || data == NULL || len == 0) return 0;

    /* 1. Put data into txbuffer (updates head) */
    uint16_t tx_len = uart->tx_rb->write(uart->tx_rb, data, len);

    /* 2. Try to transmit if UART is ready */
    /* Check if DMA is not busy. HAL_UART_Transmit_DMA checks state internally 
       but we can check gState to avoid function call overhead or logic issues */
    if (uart->huart->gState == HAL_UART_STATE_READY) {
        uart_try_transmit(uart);
    }
    return tx_len;
}

uint16_t fy_uart_rx(fy_uart_t *fy_uart, uint8_t *out, size_t len)
{
    if (fy_uart == NULL || out == NULL || len == 0) return 0;
    return (uint16_t)fy_uart->rx_rb->read(fy_uart->rx_rb, out, len);
}

void fy_uart_clear_txRb(fy_uart_t *uart)
{
    if (uart == NULL) return;
    uart->rx_rb->clear(uart->rx_rb);
}

void fy_uart_clear_rxRb(fy_uart_t *uart)
{
    if (uart == NULL) return;
    uart->rx_rb->clear(uart->rx_rb);
}

int32_t fy_uart_init(fy_uart_t *uart, UART_HandleTypeDef *huart, ringBuffer_t* rx_rb, ringBuffer_t* tx_rb)
{
    if (uart == NULL || huart == NULL || rx_rb == NULL || tx_rb == NULL) return -1;
    
    uart->huart = huart;
    uart->rx_rb = rx_rb;

    uart->tx_rb = tx_rb;
    uart->tx_active_len = 0;
    uart->uartRx = fy_uart_rx;
    uart->uartTx = fy_uart_tx;
    uart->uartClear_txBuffer = fy_uart_clear_txRb;
    uart->uartClear_rxBuffer = fy_uart_clear_rxRb;
    /* register instance so callbacks can map `huart` -> this fy_uart_t */
    if(register_uart_instance(uart) != 0) {
        return -1;
    }
    
    /* Register locks if needed (optional, user can do it outside) */

    /* Register Callbacks */
    HAL_UART_RegisterCallback(huart, HAL_UART_TX_COMPLETE_CB_ID, fy_uart_tx_cplt_callback);
    HAL_UART_RegisterCallback(huart, HAL_UART_TX_HALFCOMPLETE_CB_ID, fy_uart_tx_half_cplt_callback);
    HAL_UART_RegisterCallback(huart, HAL_UART_RX_COMPLETE_CB_ID, fy_uart_rx_cplt_callback);

    /* Start Reception */
    HAL_UART_Receive_IT(huart, &uart->rx_temp_byte, 1);
    return 0;
}









