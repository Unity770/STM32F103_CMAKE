#include "userMain.h"
#include "fy_uart.h"
#include "fy_ringBuffer.h"
#include "usart.h"
#include <string.h>
#include "elog.h"
#include "gpio.h"

//UART1相关定义
uint8_t uart1_rx_buffer[256];
uint8_t uart1_tx_buffer[256];
ringBuffer_t uart1_rx_rb;
ringBuffer_t uart1_tx_rb;
fy_uart_t uart1;

//外部中断相关定义(旋转编码器)
uint16_t enCoder_count = 0;

void uart1_init(void)
{
    ringBuffer_init(&uart1_rx_rb, uart1_rx_buffer, sizeof(uart1_rx_buffer));
    ringBuffer_init(&uart1_tx_rb, uart1_tx_buffer, sizeof(uart1_tx_buffer));
    fy_uart_init(&uart1, &huart1, &uart1_rx_rb, &uart1_tx_rb);
}

//注册给easy logger使用的输出函数
void easy_logger_out(const char *log, size_t size)
{
    uart1.uartTx(&uart1, (const uint8_t *)log, size);
}


void easy_logger_init(void)
{
    easy_logger_int_struct_t elog_init_struct = {0};
    elog_init_struct.output = easy_logger_out; // 使用默认输出函数
    elog_init_struct.output_lock = NULL; // 使用默认锁定函数
    elog_init_struct.output_unlock = NULL; // 使用默认解锁函数
    elog_init_struct.get_time = NULL; // 使用默认时间获取函数
    elog_init_struct.get_p_info = NULL; // 使用默认进程信息获取函数
    elog_init_struct.get_t_info = NULL; // 使用默认线程信息获取函数
    elog_init_struct.assert_hook = NULL; // 使用默认断言钩子函数

    elog_init(&elog_init_struct);

    elog_set_fmt(ELOG_LVL_ASSERT,  ELOG_FMT_TAG | ELOG_FMT_FUNC);
    elog_set_fmt(ELOG_LVL_ERROR,   ELOG_FMT_TAG | ELOG_FMT_FUNC);
    elog_set_fmt(ELOG_LVL_WARN,    ELOG_FMT_TAG | ELOG_FMT_FUNC);
    elog_set_fmt(ELOG_LVL_INFO,    ELOG_FMT_TAG | ELOG_FMT_FUNC);
    elog_set_fmt(ELOG_LVL_DEBUG,   ELOG_FMT_TAG | ELOG_FMT_FUNC);
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_TAG | ELOG_FMT_FUNC);

    //全部输出
    elog_set_filter_lvl(ELOG_LVL_VERBOSE);

    elog_start();
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(GPIO_Pin);
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_GPIO_EXTI_Callback could be implemented in the user file
   */
    if (GPIO_Pin == GPIO_PIN_0) // 假设使用 GPIO_PIN_0 作为外部中断引脚
    {
        // 在这里处理外部中断事件
        if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1))
        {
            if (enCoder_count < 65535)
            {
                enCoder_count++;
            }
        }
    }
    if (GPIO_Pin == GPIO_PIN_1)
    {
        if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0))
        {
            if (enCoder_count > 0)
            {
                enCoder_count--;
            }
        }
    }
    
}
void user_main(void)
{
    /* Example usage of fy_uart and ring buffer can be placed here */
    uint32_t start_tick = HAL_GetTick();
    uart1_init();
    easy_logger_init();
    while (1)
    {
        HAL_GetTick();
        if (HAL_GetTick() - start_tick >= 10)
        {
            start_tick = HAL_GetTick();
            // 每隔1秒发送一次数据
            // const char *msg = "Hello from UART1!";
            elog_a("enCoder", "count:[%d]", enCoder_count);
        }
    }
    
}