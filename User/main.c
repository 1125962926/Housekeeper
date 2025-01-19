#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/usart2/usart2.h"
#include "./SYSTEM/delay/delay.h"

#include "./BSP/LED/led.h"
#include "./BSP/KEY/key.h"
#include "./BSP/TIMER/btim.h"
#include "./BSP/EXTI/exti.h"
#include "./BSP/OLED/oled.h"
#include "./BSP/ESP8266/8266wifi.h"
#include "./BSP/RC522/RC522.h"
#include "./BSP/TIMER/gtim.h"

#include "freertos_project.h"

/* 选择esp连接方式 */
#define esp_Work_mode           mode_1_TCP_Client

/* 选择wifi模式 */
#define WIFI_Mode               wifi_mode_Station



int main(void)
{
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 设置时钟,168Mhz */
    delay_init(168);                    /* 延时初始化 */
    //usart_init(115200);                 /* 串口1初始化为115200，用于调试 */
    //usart2_init(115200);                /* 串口2初始化为115200，与esp8266通信 */
    //led_init();                         /* 初始化LED */
    OLED_Init();                        /* 初始化OLED */
    //key_init();                         /* 初始化按键 */


    /* 初始化esp8266 */
    if(esp8266_Init(esp_Work_mode,WIFI_Mode)==esp8266_OK)
    {
        switch (esp_Work_mode)
        {
            case mode_1_TCP_Client:
                printf("esp8266 工作模式: TCP客户端\r\n");
                break;
            case mode_2_UDP:
                printf("esp8266 工作模式: UDP传输\r\n");
                break;
            case mode_3_TCP_transparent:
                printf("esp8266 工作模式: TCP透传\r\n");
                break;
            case mode_4_UDP_transparent:
                printf("esp8266 工作模式: UDP透传\r\n");
                break;
            case mode_5_TCP_Server:
                printf("esp8266 工作模式: TCP服务器\r\n");
                break;
            default:
                break;
        }
    }
    else
    {
        while(1)
        {
            printf("\r\nesp8266初始化失败, 请重启开发板\r\n");
            LED0_TOGGLE();
            delay_ms(500);
        }
    }
    
    /* 初始化RFID读卡器 */
    RC522_Init();
    printf("RFID-MFRC522 初始化完成\r\n");

    /* 初始化舵机 PWM，84 000 000 / 1000 = 84 000， 84Khz的计数频率，50hz的PWM */
    gtim_timx_pwm_chy_init(1680 - 1, 1000 - 1);
    printf("舵机初始化完成\r\n");

    /* 进入FreeRTOS管理 */
    freertos_init();

    return 0;
    
}

/* 引脚分配表 */
/*
串口1：
    TX：PA9
    RX：PA10

ESP8266
    串口2：
        TX：PA2
        RX：PA3

OLED
    SCK:    PE2
    MOSI:   PE3
    RES:    PE4
    DC:     PE5

RC522
    硬件SPI1：
        RST：   PC5
        CS：    PA4
        SCK：   PA5
        MISO：  PA6
        MOSI：  PA7

舵机
    PWM：
        TIM12_ch1: PB14


*/

