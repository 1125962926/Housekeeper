#include "stdlib.h"
#include "string.h"
#include "./SYSTEM/delay/delay.h"
#include "oled.h"
#include "oledfont.h"
#include "gif.h"

/*  分辨率
	x:点的x坐标0~127
    y:点的y坐标0~63
*/


//OLED的显存
//存放格式如下.
//[0]0 1 2 3 ... 127	
//[1]0 1 2 3 ... 127	
//[2]0 1 2 3 ... 127	
//[3]0 1 2 3 ... 127	
//[4]0 1 2 3 ... 127	
//[5]0 1 2 3 ... 127	
//[6]0 1 2 3 ... 127	
//[7]0 1 2 3 ... 127 	
/* 每一列有8字节（64 / 8）数据，共有128行 */
/* 其中一列的每一字节称为一页 */
/* 刷新方向为每一列从上至下，然后每一行从左至右 */ 
uint8_t OLED_GRAM[128][8];	 

//更新显存到OLED		 
void OLED_Refresh_Gram(void)
{
	uint8_t i,n;		    
	for(i=0;i<8;i++)  
	{  
		OLED_WR_Byte (0xb0+i,OLED_CMD);    //设置页地址（0~7）
		OLED_WR_Byte (0x00,OLED_CMD);      //设置显示位置—列低地址
		OLED_WR_Byte (0x10,OLED_CMD);      //设置显示位置—列高地址   
		for(n=0;n<128;n++)OLED_WR_Byte(OLED_GRAM[n][i],OLED_DATA); 
	}   
}



/*******************************************************************
 * @name       :void OLED_Write_Byte(unsigned dat,unsigned cmd)
 * @function   :将一个字节的内容写入OLED屏幕
 * @parameters :dat:要编写的内容
                cmd:0-写入命令
								    1-写入数据
 * @retvalue   :无
********************************************************************/
void OLED_WR_Byte(uint8_t dat,uint8_t cmd)
{	
	uint8_t i;			  
	OLED_DC_PIN_Set(cmd); //写命令 	  
	for(i=0;i<8;i++)
	{			  
		OLED_SCK_PIN_SET(0);
		if(dat&0x80)
			OLED_MOSI_PIN_SET(1);
		else  
			OLED_MOSI_PIN_SET(0);
		OLED_SCK_PIN_SET(1);
		dat<<=1;   
	}				 	  
	OLED_DC_PIN_Set(1);   	  
} 

	  	  

/*******************************************************************
 * @name       :void OLED_Init_GPIO(void)
 * @function   :OLED引脚初始化
 * @parameters :无
 * @retvalue   :无
********************************************************************/ 
void OLED_Init_GPIO(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	OLED_DC_CLK_ENABLE();
	OLED_RES_CLK_ENABLE();
	OLED_SCK_CLK_ENABLE();
	OLED_MOSI_CLK_ENABLE();

	GPIO_InitStructure.Pin = OLED_DC_PIN;
 	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull=GPIO_PULLUP;		//SPI空闲时高电平，上升沿采集
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
 	HAL_GPIO_Init(OLED_DC_PORT, &GPIO_InitStructure);
 				
	GPIO_InitStructure.Pin = OLED_RES_PIN;		
 	HAL_GPIO_Init(OLED_RES_PORT, &GPIO_InitStructure);	
						
	GPIO_InitStructure.Pin = OLED_SCK_PIN;
 	HAL_GPIO_Init(OLED_SCK_PORT, &GPIO_InitStructure);
						
	GPIO_InitStructure.Pin = OLED_MOSI_PIN;		
 	HAL_GPIO_Init(OLED_MOSI_PORT, &GPIO_InitStructure);
}




/*******************************************************************
 * @name       :void OLED_Display_On(void)
 * @function   :打开OLED显示
 * @parameters :无
 * @retvalue   :无
********************************************************************/   
void OLED_Display_On(void)
{
	OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X14,OLED_CMD);  //DCDC ON
	OLED_WR_Byte(0XAF,OLED_CMD);  //DISPLAY ON
}



/*******************************************************************
 * @name       :void OLED_Display_Off(void)
 * @function   :关闭OLED显示
 * @parameters :无
 * @retvalue   :无
********************************************************************/       
void OLED_Display_Off(void)
{
	OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X10,OLED_CMD);  //DCDC OFF
	OLED_WR_Byte(0XAE,OLED_CMD);  //DISPLAY OFF
}



/*******************************************************************
 * @name       :void OLED_Clear(unsigned dat)
 * @function   :清除OLED屏幕显示
 * @retvalue   :无
********************************************************************/  
void OLED_Clear(void)  
{  
	uint8_t i,n;  
	for(i=0;i<8;i++)for(n=0;n<128;n++)OLED_GRAM[n][i]=0X00;  
	OLED_Refresh_Gram();//更新显示
}




/*******************************************************************
 * @name       :void OLED_DrawPoint(u8 x,u8 y,u8 color)
 * @function   :在OLED屏幕中绘制点
 * @parameters :x:点的x坐标0~127
                y:点的y坐标0~63
				t:1 填充 0,清空
 * @retvalue   :无
********************************************************************/			   
void OLED_DrawPoint(uint8_t x,uint8_t y,uint8_t t)
{
	uint8_t pos,bx,temp=0;
	if(x>127||y>63)return;//超出范围了.
	pos=7-y/8;
	bx=y%8;
	temp=1<<(7-bx);
	if(t)OLED_GRAM[x][pos]|=temp;
	else OLED_GRAM[x][pos]&=~temp;	    
}




/*******************************************************************
 * @name       :void OLED_Fill(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2,uint8_t dot)
 * @function   :填充指定区域
 * @parameters :x1:指定填充区域的起始x坐标
                y1:指定填充区域的起始y坐标
				x2:指定填充区域的结束x坐标
				y2:指定填充区域的结束y坐标
				dot:0,清空;1,填充
 * @retvalue   :无
********************************************************************/  
void OLED_Fill(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2,uint8_t dot)  
{  
	uint8_t x,y;  
	for(x=x1;x<=x2;x++)
	{
		for(y=y1;y<=y2;y++)OLED_DrawPoint(x,y,dot);
	}													    
	OLED_Refresh_Gram();//更新显示
}



/*****************************************************************************
 * @name       :void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t size,uint8_t mode)
 * @function   :显示单个英文字符
 * @parameters :x:字符显示位置的起始x坐标0~127
                y:字符显示位置的起始y坐标0~63
				chr:显示字符的ascii码（0～94）
				size:选择字体 12/16/24
				mode:0,反白显示;1,正常显示

 * @retvalue   :无
******************************************************************************/
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t size,uint8_t mode)
{      			    
	uint8_t temp,t,t1;
	uint8_t y0=y;
	uint8_t csize=(size/8+((size%8)?1:0))*(size/2);		//得到字体一个字符对应点阵集所占的字节数
	chr=chr-' ';//得到偏移后的值		 
    for(t=0;t<csize;t++)
    {   
		if(size==12)temp=asc2_1206[chr][t]; 	 	//调用1206字体
		else if(size==16)temp=asc2_1608[chr][t];	//调用1608字体
		else if(size==24)temp=asc2_2412[chr][t];	//调用2412字体
		else return;								//没有的字库
        for(t1=0;t1<8;t1++)
		{
			if(temp&0x80)OLED_DrawPoint(x,y,mode);
			else OLED_DrawPoint(x,y,!mode);
			temp<<=1;
			y++;
			if((y-y0)==size)
			{
				y=y0;
				x++;
				break;
			}
		}  	 
    }          
}

//在指定位置显示一个中文字符
//x:
//y:
//mode:0,反白显示;1,正常显示				 
//size:选择字体 12/16/24
/*****************************************************************************
 * @name       :void OLED_Chinese_Text(uint8_t x,uint8_t y,uint8_t chr,uint8_t size,uint8_t mode) 
 * @function   :显示中文字符串
 * @parameters :x:汉语字符串的起始x坐标0~127
                y:汉语字符串的起始y坐标0~63
				chr:二位字库中的列向量，第几个字
				size:中文字符串的大小
				mode:0,反白显示;1,正常显示

 * @retvalue   :无
******************************************************************************/
void OLED_Chinese_Text(uint8_t x,uint8_t y,uint8_t chr,uint8_t size,uint8_t mode)
{      			    
	uint8_t temp,t,t1;
	uint8_t y0=y;
	uint8_t csize=(size/8+((size%8)?1:0))*size;		//得到字体一个字符对应点阵集所占的字节数		 
    for(t=0;t<csize;t++)
    {   
		temp = Chinese_Text[chr][t];								
        for(t1=0;t1<8;t1++)
		{
			if(temp&0x80)OLED_DrawPoint(x,y,mode);
			else OLED_DrawPoint(x,y,!mode);
			temp<<=1;
			y++;
			if((y-y0)==size)
			{
				y=y0;
				x++;
				break;
			}
		}  	 
    }          
}


/*****************************************************************************
 * @name       :OLED_DrawBMP
 * @function   :显示显示BMP图片128×64起始点坐标(x,y)
 * @parameters :x的范围0～127，
                y的范围0~63
				page：图片帧号，第几帧
				image_x：图像长
				image_y：图像宽
******************************************************************************/
void OLED_DrawBMP(uint8_t x, uint8_t y, uint8_t page, uint8_t image_x, uint8_t image_y)
{ 	
	uint8_t point_byte;/* 图像数组每一字节 */
	uint8_t t2, t1;
	uint8_t y0 = y;
	uint8_t csize = (image_y / 8 + ((image_y % 8) ? 1 : 0)) * image_x;		//得到每一帧所占的字节数
		 
    for(t2 = 0; t2 < csize; t2++)/* 按字节写入 */
    {
		point_byte = GIF_image[page][t2];						
        for(t1 = 0; t1 < 8; t1++)/* 字节内每一像素点 */
		{
			if(point_byte & 0x80)
				OLED_DrawPoint(x,y,1);
			else
				OLED_DrawPoint(x,y,0);
			point_byte<<=1;
			y++;
			if((y - y0) == image_y)
			{
				y = y0;
				x++;
				break;
			}
		}  	 
    } 
} 


/*****************************************************************************
 * @name       :u32 mypow(uint8_t m,uint8_t n)
 * @function   :获取m的n次方（内部调用）
 * @parameters :m:the multiplier
                n:the power
 * @retvalue   :m的n次方
******************************************************************************/
uint32_t mypow(uint8_t m,uint8_t n)
{
	uint32_t result=1;	 
	while(n--)result*=m;    
	return result;
}	




/*****************************************************************************
 * @name       :void OLED_ShowNum(uint8_t x,uint8_t y,u32 num,uint8_t len,uint8_t size)
 * @function   :显示数字
 * @parameters :x:数字的起始x坐标
                y:数字的起始y坐标
				num:数字（0～4294967295）
				len:显示数字的长度
				Size:显示数字的大小

 * @retvalue   :无
******************************************************************************/	 		  
void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t size)
{         	
	uint8_t t,temp;
	uint8_t enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar(x+(size/2)*t,y,' ',size,1);
				continue;
			}else enshow=1; 
		 	 
		}
	 	OLED_ShowChar(x+(size/2)*t,y,temp+'0',size,1); 
	}
} 



 
/*****************************************************************************
 * @name       :void OLED_ShowString(uint8_t x,uint8_t y,const uint8_t *p,uint8_t size) 
 * @function   :显示英文字符串
 * @parameters :x:英语字符串的起始x坐标
                y:英语字符串的起始y坐标
				*p:字符串起始地址
				size:显示字符的大小
 * @retvalue   :无
******************************************************************************/
void OLED_ShowString(uint8_t x,uint8_t y,const uint8_t *p,uint8_t size)
{	
    while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
    {       
        if(x>(128-(size/2))){x=0;y+=size;}
        if(y>(64-size)){y=x=0;OLED_Clear();}
        OLED_ShowChar(x,y,*p,size,1);	 
        x+=size/2;
        p++;
    }  
	
}	



/*******************************************************************
 * @name       :void OLED_Init(void)
 * @function   :初始化OLED SSD1306控制IC
 * @parameters :无
 * @retvalue   :无
********************************************************************/				    
void OLED_Init(void)
{ 	 	 			 	 				 
	OLED_Init_GPIO();
									  
	OLED_DC_PIN_Set(1);	 
	OLED_RST_PIN_Set(0);
	delay_ms(100);
	OLED_RST_PIN_Set(1); 
					  
	OLED_WR_Byte(0xAE,OLED_CMD); //关闭显示
	OLED_WR_Byte(0xD5,OLED_CMD); //设置时钟分频因子,震荡频率
	OLED_WR_Byte(80,OLED_CMD);   //[3:0],分频因子;[7:4],震荡频率
	OLED_WR_Byte(0xA8,OLED_CMD); //设置驱动路数
	OLED_WR_Byte(0X3F,OLED_CMD); //默认0X3F(1/64) 
	OLED_WR_Byte(0xD3,OLED_CMD); //设置显示偏移
	OLED_WR_Byte(0X00,OLED_CMD); //默认为0

	OLED_WR_Byte(0x40,OLED_CMD); //设置显示开始行 [5:0],行数.
													    
	OLED_WR_Byte(0x8D,OLED_CMD); //电荷泵设置
	OLED_WR_Byte(0x14,OLED_CMD); //bit2，开启/关闭
	OLED_WR_Byte(0x20,OLED_CMD); //设置内存地址模式
	OLED_WR_Byte(0x02,OLED_CMD); //[1:0],00，列地址模式;01，行地址模式;10,页地址模式;默认10;
	OLED_WR_Byte(0xA1,OLED_CMD); //段重定义设置,bit0:0,0->0;1,0->127;
	OLED_WR_Byte(0xC0,OLED_CMD); //设置COM扫描方向;bit3:0,普通模式;1,重定义模式 COM[N-1]->COM0;N:驱动路数
	OLED_WR_Byte(0xDA,OLED_CMD); //设置COM硬件引脚配置
	OLED_WR_Byte(0x12,OLED_CMD); //[5:4]配置
		 
	OLED_WR_Byte(0x81,OLED_CMD); //对比度设置
	OLED_WR_Byte(0xEF,OLED_CMD); //1~255;默认0X7F (亮度设置,越大越亮)
	OLED_WR_Byte(0xD9,OLED_CMD); //设置预充电周期
	OLED_WR_Byte(0xf1,OLED_CMD); //[3:0],PHASE 1;[7:4],PHASE 2;
	OLED_WR_Byte(0xDB,OLED_CMD); //设置VCOMH 电压倍率
	OLED_WR_Byte(0x30,OLED_CMD); //[6:4] 000,0.65*vcc;001,0.77*vcc;011,0.83*vcc;

	OLED_WR_Byte(0xA4,OLED_CMD); //全局显示开启;bit0:1,开启;0,关闭;(白屏/黑屏)
	OLED_WR_Byte(0xA6,OLED_CMD); //设置显示方式;bit0:1,反相显示;0,正常显示	    						   
	OLED_WR_Byte(0xAF,OLED_CMD); //开启显示	 
	OLED_Clear();
}  


