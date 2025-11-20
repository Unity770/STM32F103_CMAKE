#include "fy_uart.h"

/* Private variables ---------------------------------------------------------*/
static uint8_t tx_buffer_storage[512];
static uint8_t rx_buffer_storage[512];
static fy_uart_t *g_fy_uart_handle = NULL;

/* Private function prototypes -----------------------------------------------*/
static void uart_try_transmit(fy_uart_t *uart);
static void fy_uart_tx_cplt_callback(UART_HandleTypeDef *huart);
static void fy_uart_tx_half_cplt_callback(UART_HandleTypeDef *huart);
static void fy_uart_rx_cplt_callback(UART_HandleTypeDef *huart);

/* Exported functions --------------------------------------------------------*/

void fy_uart_init(fy_uart_t *uart, UART_HandleTypeDef *huart)
{
    if (uart == NULL || huart == NULL) return;
    
    uart->huart = huart;
    g_fy_uart_handle = uart; // Simple single-instance support for callback

    /* Initialize RingBuffers */
    ringBuffer_init(&uart->tx_rb, tx_buffer_storage, sizeof(tx_buffer_storage));
    ringBuffer_init(&uart->rx_rb, rx_buffer_storage, sizeof(rx_buffer_storage));
    
    /* Register locks if needed (optional, user can do it outside) */

    /* Register Callbacks */
    HAL_UART_RegisterCallback(huart, HAL_UART_TX_COMPLETE_CB_ID, fy_uart_tx_cplt_callback);
    HAL_UART_RegisterCallback(huart, HAL_UART_TX_HALFCOMPLETE_CB_ID, fy_uart_tx_half_cplt_callback);
    HAL_UART_RegisterCallback(huart, HAL_UART_RX_COMPLETE_CB_ID, fy_uart_rx_cplt_callback);

    /* Start Reception */
    HAL_UART_Receive_IT(huart, &uart->rx_temp_byte, 1);
}

void fy_uart_tx(fy_uart_t *uart, const uint8_t *data, size_t len)
{
    if (uart == NULL || data == NULL || len == 0) return;

    /* 1. Put data into txbuffer (updates head) */
    uart->tx_rb.write(&uart->tx_rb, data, len);

    /* 2. Try to transmit if UART is ready */
    /* Check if DMA is not busy. HAL_UART_Transmit_DMA checks state internally 
       but we can check gState to avoid function call overhead or logic issues */
    if (uart->huart->gState == HAL_UART_STATE_READY) {
        uart_try_transmit(uart);
    }
}

uint16_t fy_uart_rx(fy_uart_t *fy_uart, uint8_t *out, size_t len)
{
    if (fy_uart == NULL || out == NULL || len == 0) return 0;
    return (uint16_t)fy_uart->rx_rb.read(&fy_uart->rx_rb, out, len);
}

/* Private functions ---------------------------------------------------------*/

static void uart_try_transmit(fy_uart_t *uart)
{
    ringBuffer_t *rb = &uart->tx_rb;
    
    /* Check buffer count */
    size_t head = rb->head;
    size_t tail = rb->tail;
    size_t size = rb->size;
    size_t len_to_send = 0;

    /* Calculate contiguous data length from tail */
    if (head > tail) {
        len_to_send = head - tail;
    } else if (head < tail) {
        len_to_send = size - tail; /* Send until end of buffer */
    }

    if (len_to_send > 0) {
        uart->tx_active_len = len_to_send;
        /* Call HAL_UART_Transmit_DMA */
        HAL_UART_Transmit_DMA(uart->huart, &rb->buffer[tail], len_to_send);
    }
}

/* HAL Callbacks -------------------------------------------------------------*/

static void fy_uart_tx_cplt_callback(UART_HandleTypeDef *huart)
{
    /* Check if this is our uart */
    if (g_fy_uart_handle != NULL && g_fy_uart_handle->huart == huart) {
        fy_uart_t *uart = g_fy_uart_handle;
        ringBuffer_t *rb = &uart->tx_rb;

        /* Update tail for the remaining part */
        size_t half = uart->tx_active_len / 2;
        size_t remaining = uart->tx_active_len - half;
        
        rb->tail = (rb->tail + remaining) % rb->size;
        uart->tx_active_len = 0;

        /* Check buffer count and transmit next chunk if available */
        uart_try_transmit(uart);
    }
}

static void fy_uart_tx_half_cplt_callback(UART_HandleTypeDef *huart)
{
    if (g_fy_uart_handle != NULL && g_fy_uart_handle->huart == huart) {
        fy_uart_t *uart = g_fy_uart_handle;
        ringBuffer_t *rb = &uart->tx_rb;

        /* Update tail for the first half */
        size_t half = uart->tx_active_len / 2;
        if (half > 0) {
            rb->tail = (rb->tail + half) % rb->size;
        }
    }
}

static void fy_uart_rx_cplt_callback(UART_HandleTypeDef *huart)
{
    if (g_fy_uart_handle != NULL && g_fy_uart_handle->huart == huart) {
        fy_uart_t *uart = g_fy_uart_handle;
        
        /* Push received byte to ringbuffer */
        uart->rx_rb.write(&uart->rx_rb, &uart->rx_temp_byte, 1);

        /* Restart reception */
        HAL_UART_Receive_IT(huart, &uart->rx_temp_byte, 1);
    }
}



