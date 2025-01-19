#ifndef __USART2_H
#define __USART2_H

#include "stdio.h"
#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/
/* 引脚 和 串口 定义 
 */
#define USART2_TX_GPIO_PORT              GPIOA
#define USART2_TX_GPIO_PIN               GPIO_PIN_2
#define USART2_TX_GPIO_AF                GPIO_AF7_USART2
#define USART2_TX_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* 发送引脚时钟使能 */

#define USART2_RX_GPIO_PORT              GPIOA
#define USART2_RX_GPIO_PIN               GPIO_PIN_3
#define USART2_RX_GPIO_AF                GPIO_AF7_USART2
#define USART2_RX_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* 接收引脚时钟使能 */

#define USART2_UX                            USART2
#define USART2_UX_IRQn                       USART2_IRQn
#define USART2_UX_IRQHandler                 USART2_IRQHandler
#define USART2_UX_CLK_ENABLE()               do{ __HAL_RCC_USART2_CLK_ENABLE(); }while(0)  /* USART2 时钟使能 */


/******************************************************************************************/

#define USART2_REC_LEN               200         /* 定义最大接收字节数 200 */
#define USART2_EN_RX                 1           /* 使能（1）/禁止（0）串口1接收 */
#define RXBUFFERSIZE_2   1                        /* 缓存大小 */

extern UART_HandleTypeDef g_uart2_handle;       /* HAL UART句柄 */

extern uint8_t  g_usart2_rx_buf[USART2_REC_LEN];  /* 接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 */
extern uint16_t g_usart2_rx_sta;                 /* 接收状态标记 */
extern uint8_t g_rx_buffer2[RXBUFFERSIZE_2];       /* USART2接收Buffer */


void usart2_init(uint32_t bound);                /* 串口初始化函数 */
void usart2_send_string(char *str);              /* 串口发送字符串 */

#endif


