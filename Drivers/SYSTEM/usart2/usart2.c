#include "usart2.h"
#include "string.h"


#if USART2_EN_RX /*如果使能了接收*/

/* 接收缓冲, 最大USART_REC_LEN个字节. */
uint8_t g_usart2_rx_buf[USART2_REC_LEN];

/*  接收状态
 *  bit15，      接收完成标志
 *  bit14，      接收到0x0d
 *  bit13~0，    接收到的有效字节数目
*/
uint16_t g_usart2_rx_sta = 0;

uint8_t g_rx_buffer2[RXBUFFERSIZE_2];  /* HAL库使用的串口接收缓冲 */

UART_HandleTypeDef g_uart2_handle;  /* UART句柄 */

/**
 * @brief       串口X初始化函数
 * @param       baudrate: 波特率, 根据自己需要设置波特率值
 * @note        注意: 必须设置正确的时钟源, 否则串口波特率就会设置异常.
 *              这里的USART的时钟源在sys_stm32_clock_init()函数中已经设置过了.
 * @retval      无
 */
void usart2_init(uint32_t baudrate)
{
    /*UART 初始化设置*/
    g_uart2_handle.Instance = USART2_UX;                                       /* USART_UX */
    g_uart2_handle.Init.BaudRate = baudrate;                                  /* 波特率 */
    g_uart2_handle.Init.WordLength = UART_WORDLENGTH_8B;                      /* 字长为8位数据格式 */
    g_uart2_handle.Init.StopBits = UART_STOPBITS_1;                           /* 一个停止位 */
    g_uart2_handle.Init.Parity = UART_PARITY_NONE;                            /* 无奇偶校验位 */
    g_uart2_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;                      /* 无硬件流控 */
    g_uart2_handle.Init.Mode = UART_MODE_TX_RX;                               /* 收发模式 */
    HAL_UART_Init(&g_uart2_handle);                                           /* HAL_UART_Init()会使能UART1 */

    /* 该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量 */
    HAL_UART_Receive_IT(&g_uart2_handle, (uint8_t *)g_rx_buffer2, RXBUFFERSIZE_2); 
}



/**
 * @brief       串口2中断服务函数
                注意,读取USARTx->SR能避免莫名其妙的错误
 * @param       无
 * @retval      无
 */
void USART2_UX_IRQHandler(void)
{

    HAL_UART_IRQHandler(&g_uart2_handle);                               /* 调用HAL库中断处理公用函数 */

    while (HAL_UART_Receive_IT(&g_uart2_handle, (uint8_t *)g_rx_buffer2, RXBUFFERSIZE_2) != HAL_OK)     /* 重新开启中断并接收数据 */
    {
        /* 如果出错会卡死在这里 */
        
    }

}


/**
 * @brief       串口2发送字符串

 * @param       *str：要发送的字符串
 * @retval      无
 */
void usart2_send_string(char *str)
{

    /* 发送字符串 */
    HAL_UART_Transmit(&g_uart2_handle, (uint8_t *)str, strlen((const char *)str), 0xffff);  /* 发送数据 */

}
// if ((g_usart2_rx_sta & 0x8000) == 0x8000)//接收到一次数据
// {
//     uint8_t len = g_usart2_rx_sta & 0X3FFF;
//     for(int i=0;i<len;i++)
//     {
//         ch = g_usart2_rx_buf[i];
//         while ((USART2_UX->SR & 0X40) == 0);     /* 等待上一个字符发送完成 */

//         USART2_UX->DR = ch;             /* 将要发送的字符 ch 写入到DR寄存器 */

//     }
//     g_usart2_rx_sta = 0;
// }
#endif
