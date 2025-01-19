#ifndef __OLED_H
#define __OLED_H		


#include "./SYSTEM/sys/sys.h"
	 
/* -----------------GIF数据---------------- */
/* GIF 帧数 */
#define page_sum                18

/* 帧长 */
#define frame_len                114

/* 帧宽 */
#define frame_width              64

/* 刷新速度，间隔多少ms */
#define refresh_speed            200

/* -------------写命令和数据定义------------------- */
#define OLED_CMD			0		//写命令
#define OLED_DATA			1		//写数据 


/* -----------------OLED端口定义---------------- */
/* SPI协议由软件模拟实现，引脚可任意指定 */
#define OLED_SCK_PIN		GPIO_PIN_2							//时钟信号
#define OLED_SCK_PORT		GPIOE										
#define OLED_SCK_CLK_ENABLE()      do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0)

#define OLED_MOSI_PIN		GPIO_PIN_3							//数据发送信号
#define OLED_MOSI_PORT	    GPIOE										
#define OLED_MOSI_CLK_ENABLE()      do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0)

#define OLED_RES_PIN		GPIO_PIN_4							//复位信号
#define OLED_RES_PORT		GPIOE										
#define OLED_RES_CLK_ENABLE()      do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0)

#define OLED_DC_PIN			GPIO_PIN_5							//数据/命令控制信号
#define OLED_DC_PORT		GPIOE										
#define OLED_DC_CLK_ENABLE()      do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0)	



/* -----------------OLED端口操作定义---------------- */

#define OLED_RST_PIN_Set(x) do{x ? HAL_GPIO_WritePin(OLED_RES_PORT,OLED_RES_PIN,GPIO_PIN_SET) : \
                            HAL_GPIO_WritePin(OLED_RES_PORT,OLED_RES_PIN,GPIO_PIN_RESET); } while(0)

#define OLED_DC_PIN_Set(x) do{x ? HAL_GPIO_WritePin(OLED_DC_PORT,OLED_DC_PIN,GPIO_PIN_SET) : \
                            HAL_GPIO_WritePin(OLED_DC_PORT,OLED_DC_PIN,GPIO_PIN_RESET); } while(0)

#define OLED_SCK_PIN_SET(x) do{x ? HAL_GPIO_WritePin(OLED_SCK_PORT,OLED_SCK_PIN,GPIO_PIN_SET) : \
                            HAL_GPIO_WritePin(OLED_SCK_PORT,OLED_SCK_PIN,GPIO_PIN_RESET); } while(0)

#define OLED_MOSI_PIN_SET(x) do{x ? HAL_GPIO_WritePin(OLED_MOSI_PORT,OLED_MOSI_PIN,GPIO_PIN_SET) : \
                            HAL_GPIO_WritePin(OLED_MOSI_PORT,OLED_MOSI_PIN,GPIO_PIN_RESET) ; } while(0)

/**********************************************/
/* 内部调用 */
void OLED_WR_Byte(uint8_t dat,uint8_t cmd);	    
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_Init_GPIO(void);
/**********************************************/

/* 刷新OLED */
void OLED_Refresh_Gram(void);

/* 初始化 */
void OLED_Init(void);

/* 纯色填充 */
void OLED_Fill(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2,uint8_t dot);

/* 清屏 */
void OLED_Clear(void);

/* 打点 */
void OLED_DrawPoint(uint8_t x,uint8_t y,uint8_t t);

/**********************************************/
/* 显示API */
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t size,uint8_t mode);
void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t size);
void OLED_ShowString(uint8_t x,uint8_t y,const uint8_t *p,uint8_t size);
void OLED_Chinese_Text(uint8_t x,uint8_t y,uint8_t chr,uint8_t size,uint8_t mode); 																								//英文天气界面显示测试
void OLED_DrawBMP(uint8_t x, uint8_t y, uint8_t page, uint8_t image_x, uint8_t image_y);

#endif


