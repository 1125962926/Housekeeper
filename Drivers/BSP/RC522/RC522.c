#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "./BSP/RC522/RC522.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/SPI/spi.h"
#include "./BSP/OLED/oled.h"

/******************************************************************************/
/* 卡片相关数据 */

static uint8_t Card_password[6]={0xff,0xff,0xff,0xff,0xff,0xff};    /* 验证密码 */
static uint8_t IC_UID[4];                                           /* UID卡片序列号,4字节，在 PcdAnticoll 函数中读出 */
static uint8_t IC_Type[2];                                          /* 卡类型代码，2字节,0x0400，Mifare_One(S50) */

/* 数据库内的卡号，可开门卡号 */
#define CARD_NUM 2                                  /* RFID卡数量 */
static uint8_t card_ID[][4]={
    {0xC3,0xB2,0x37,0xC5},                          /* 蓝卡1卡号 */
    {0xA3,0x09,0x3C,0xFB}                           /* 白卡2卡号 */
    };
    
static uint8_t ID_num=0;                            /* 当前操作的卡 */


/******************************  初始化函数  *******************************************/

/* M1卡分为16个扇区，每个扇区由四个块（块0、块1、块2、块3）组成
 * 将16个扇区的64个块按绝对地址编号为：0~63,每块16个字节
 * 第0个扇区的块0（即绝对地址0块），用于存放厂商代码，已经固化不可更改 
 * 每个扇区的块0、块1、块2为数据块，可用于存放数据
 * 每个扇区的块3为控制块（绝对地址为:块3、块7、块11.....）包括密码A，存取控制、密码B等
 */ 

void RC522_Init(void)
{

	RC522_SPI_GPIO_Init();//初始化RC522的复位引脚和片选引脚

	SPI1_Init();//初始化SPI	

    delay_ms(50);
	PcdReset();//复位RC522读卡器

	delay_ms(10);
	PcdAntennaOff();//关闭天线发射
	
	delay_ms(10);
    PcdAntennaOn();//开启天线发射

}

/******************************  FreeRTOS  *******************************************/
/*
 * @brief       等待刷卡
 * @param       *data：要发送的数据
 * @param       ID：要收到数据的ID号
 * @retval      result：成功返回MI_OK，其他失败
 */
uint8_t Wait_RFID_card(void)
{
    /* 寻卡 */
    return PcdRequest(PICC_REQALL, IC_Type);
}


/*
 * @brief       读卡，返回卡号
 * @param       *data：要发送的数据
 * @param       ID：要收到数据的ID号
 * @retval      result：返回1成功，0则失败
 */
int8_t GET_card_ID(void)
{
    /* 防冲撞，读出卡号 */
    if(PcdAnticoll(IC_UID) != MI_OK)
    {
        printf("防冲撞失败，请重试\r\n");
        return -1;
    }

    /* 返回卡号 */
    return find_RFID_card();
}


/******************************  测试函数  *******************************************/
static uint8_t Read_Card_Data[16];                       /* 读取出的数据 */
/**
 * @brief       读卡或写卡
 * @param       block：要操作的块地址
 * @param       option：读操作或写操作，readID、writeID
 * @param       Write_Card_Data：要写入的数据
 */
void RC522_Start(uint8_t block,uint8_t option,uint8_t *Write_Card_Data)
{
	
	if(PcdRequest(PICC_REQALL,IC_Type) == MI_OK)//寻卡
	{		
		uint16_t cardType = (IC_Type[0] << 8) | IC_Type[1];
        switch (cardType)
        {
            case 0x4400:
                printf("\r\nMifare UltraLight\r\n");
                break;
            case 0x0400:
                printf("\r\nMifare One(S50)\r\n");
                break;
            case 0x0200:
                printf("\r\nMifare One(S70)\r\n");
                break;
            case 0x0800:
                printf("\r\nMifare Pro(X)\r\n");
                break;
            case 0x4403:
                printf("\r\nMifare DESFire\r\n");
                break;
            default:
                printf("\r\nUnknown Card\r\n");
                break;
        }
		if(PcdAnticoll(IC_UID)==MI_OK)//防冲撞
		{
			ID_num = find_RFID_card();
            if(ID_num == 0)
            {                
                printf("The card is not exist!\r\n");
                exit(0);
            }                
            printf("The card is: %d", ID_num);
            printf("\r\ncard_ID: %02X:%02X:%02X:%02X\r\n",IC_UID[0],IC_UID[1],IC_UID[2],IC_UID[3]); //打印卡的UID号
			
			if(PcdSelect(IC_UID)==MI_OK)//选卡
			{
				if(PcdAuthState(PICC_AUTHENT1A,block,Card_password,IC_UID)==MI_OK)//验证A密钥,对应块
				{									
					if(option==readID)//读操作
					{
						if(PcdRead(block,Read_Card_Data)==MI_OK)
						{
							printf("\n读取结果: \n");
							printf("block %d date:\r\n",block);
							for(int i=0;i<16;i++)
							{
								printf("%02X ",Read_Card_Data[i]);
							} 
							printf("\r\n");                     
							
						}
					}
					else if(option==writeID)//写操作
					{						
						if(PcdWrite(block,Write_Card_Data)==MI_OK)
						{
							delay_ms(8);
							if(PcdRead(block,Read_Card_Data)==MI_OK)
							{
								printf("\n写入完成\n");
								printf("block %d date:\r\n",block);
								for(int i=0;i<16;i++)
								{
									printf("%02X ",Read_Card_Data[i]);
								} 
								printf("\r\n");                     								
							}
						}
					}	
				}
				else
				{
					printf("\r\n验证失败\r\n");
				}
			}
			else
			{
				printf("\r\n选卡失败\r\n");
			}
		}
		else
		{
			printf("\r\n防冲撞失败\r\n");
		}	
	}
	else
	{
		printf("\r\n寻卡失败\r\n");
	}
	PcdAntennaOff();//关闭天线发射
			
	delay_ms(10);
	PcdAntennaOn();//开启天线发射	
}


/******************************  内部调用  *******************************************/
/*
 * 函数名：RC522_SPI_GPIO_Init
 * 描述  ：初始化RC522的复位引脚和片选引脚
 *         片选引脚采用软件管理模式，自定义片选引脚
 */
void RC522_SPI_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    
    RC522_GPIO_Reset_CLK_ENABLE();
    RC522_GPIO_CS_CLK_ENABLE();
    
    GPIO_Initure.Pin=RC522_RST_Pin;
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;
    GPIO_Initure.Pull=GPIO_PULLUP;     //复位和片选引脚低电平有效,默认上拉             
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_HIGH;                    
    HAL_GPIO_Init(RC522_RST_GPIO_Port, &GPIO_Initure);

    GPIO_Initure.Pin=RC522_GPIO_CS_PIN;
    HAL_GPIO_Init(RC522_GPIO_CS_PORT, &GPIO_Initure);
}

/**
 * @brief       返回卡号
 * @param       *data：要发送的数据
 * @param       ID：要收到数据的ID号
 * @retval      成功返回卡号，失败返回 0
 */
uint8_t find_RFID_card(void)
{   
    uint8_t result = 0;
    for (uint8_t i = 0; i < CARD_NUM; i++)
    {
        for (uint8_t j = 0; j < 4; j++)/* 对比每个字节 */
        {
            if (IC_UID[j] != card_ID[i][j])
            {
                result = 0;
                break;
            }
            result = 1;
        }

        if (!result)
        {
            result = 0;
        }
        else/* 在数据库内 */
        {
            return i + 1;
        }
    }
    return 0;
}


/*
 * 函数名：SPI_WriteNBytes
 * 描述  ：向RC522发送n Byte 数据
 * 输入  ：SPIx : 要发送数据的SPI
 * 输入  ：p_TxData : 要发送的数据
 * 输入  ：sendDataNum : 要发送的数据量(Byte)
 * 返回  : 0
 */
int SPI_WriteNBytes(SPI_TypeDef* SPIx, uint8_t *p_TxData,uint32_t sendDataNum)
{
	int retry=0;
	while(sendDataNum--){
		while((SPIx->SR&SPI_FLAG_TXE)==0)//等待发送区空
		{
			retry++;
			if(retry>20000)return -1;
		}
		SPIx->DR=*p_TxData++;//发送一个byte
		retry=0;
		while((SPIx->SR&SPI_FLAG_RXNE)==0)//等待接收完一个byte
		{
			SPIx->SR = SPIx->SR;
			retry++;
			if(retry>20000)return -1;
		}
		SPIx->DR;
	}
    return 0;
}



/*
 * 函数名：SPI_ReadNBytes
 * 描述  ：读取RC522 n Byte 数据
 * 输入  ：SPIx : 要读取数据的SPI
 * 输入  ：p_RxData : 要读取的数据
 * 输入  ：readDataNum : 要读取的数据量(Byte)
 * 返回  : 0
 */
int SPI_ReadNBytes(SPI_TypeDef* SPIx, uint8_t *p_RxData,uint32_t readDataNum)
{
	int retry=0;
	while(readDataNum--){
		SPIx->DR = 0xFF;
		while(!(SPIx->SR&SPI_FLAG_TXE)){
			retry++;
			if(retry>20000)return -1;
		}
		retry = 0;
		while(!(SPIx->SR&SPI_FLAG_RXNE)){
			retry++;
			if(retry>20000)return -1;
		}
		*p_RxData++ = SPIx->DR;
	}
    return 0;
}



/*
 * 函数名：PcdComMF522
 * 描述  ：通过RC522和ISO14443卡通讯
 * 输入  ：ucCommand，RC522命令字
 *         pInData，通过RC522发送到卡片的数据
 *         ucInLenByte，发送数据的字节长度
 *         pOutData，接收到的卡片返回数据
 *         pOutLenBit，返回数据的位长度
 * 返回  : 状态值
 *         = MI_OK，成功
 * 调用  ：内部调用
 */
char PcdComMF522(uint8_t Command,uint8_t *pInData,uint8_t InLenByte,uint8_t *pOutData,uint32_t *pOutLenBit)
{
    char status = MI_ERR;
    uint8_t irqEn = 0x00;
    uint8_t waitFor = 0x00;
    uint8_t lastBits;
    uint8_t n;
    uint32_t i;
    switch (Command)
    {
    case PCD_AUTHENT:   //Mifare认证
        irqEn = 0x12;   //允许错误中断请求ErrIEn  允许空闲中断IdleIEn
        waitFor = 0x10; //认证寻卡等待时候 查询空闲中断标志位
        break;
    case PCD_TRANSCEIVE://接收发送 发送接收
        irqEn = 0x77;   //允许TxIEn RxIEn IdleIEn LoAlertIEn ErrIEn TimerIEn
        waitFor = 0x30; //寻卡等待时候 查询接收中断标志位与 空闲中断标志位
        break;
    default:
        break;
    }
    WriteRawRC(ComIEnReg, irqEn | 0x80); //IRqInv置位管脚IRQ与Status1Reg的IRq位的值相反
    ClearBitMask(ComIrqReg, 0x80);       //Set1该位清零时，CommIRqReg的屏蔽位清零
    WriteRawRC(CommandReg, PCD_IDLE);    //写空闲命令
    SetBitMask(FIFOLevelReg, 0x80);      //置位FlushBuffer清除内部FIFO的读和写指针以及ErrReg的BufferOvfl标志位被清除
    for (i = 0; i < InLenByte; i++)
    {
        WriteRawRC(FIFODataReg, pInData[i]);//写数据进FIFOdata
    }
    WriteRawRC(CommandReg, Command);//写命令 
    if (Command == PCD_TRANSCEIVE)
    {
        SetBitMask(BitFramingReg, 0x80);//StartSend置位启动数据发送 该位与收发命令使用时才有效
    }
    i = 800;//根据时钟频率调整，操作M1卡最大等待时间25ms
    do
    {
        n = ReadRawRC(ComIrqReg);//查询事件中断
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitFor));//退出条件i=0,定时器中断，与写空闲命令
    ClearBitMask(BitFramingReg, 0x80);//清理允许StartSend位
    if (i != 0)
    {
        if (!(ReadRawRC(ErrorReg) & 0x1B))//读错误标志寄存器BufferOfI CollErr ParityErr ProtocolErr
        {
            status = MI_OK;
            if (n & irqEn & 0x01)//是否发生定时器中断
            {
                status = MI_NOTAGERR;
            }
            if (Command == PCD_TRANSCEIVE)
            {
                n = ReadRawRC(FIFOLevelReg);//读FIFO中保存的字节数
                lastBits = ReadRawRC(ControlReg) & 0x07;//最后接收到得字节的有效位数
                if (lastBits)
                {
                    *pOutLenBit = (n - 1) * 8 + lastBits;//N个字节数减去1（最后一个字节）+最后一位的位数 读取到的数据总位数
                }
                else
                {
                    *pOutLenBit = n * 8;//最后接收到的字节整个字节有效
                }
                if (n == 0)
                {
                    n = 1;
                }
                if (n > MAXRLEN)
                {
                    n = MAXRLEN;
                }
                for (i = 0; i < n; i++)
                {
                    pOutData[i] = ReadRawRC(FIFODataReg);
                }
            }
        }
        else
        {
            status = MI_ERR;
        }
    }
    SetBitMask(ControlReg, 0x80); // stop timer now
    WriteRawRC(CommandReg, PCD_IDLE);
    return status;
}

/*
 * 函数名：PcdRequest
 * 描述  ：寻卡
 * 输入  ：ucReq_code，寻卡方式
 *                     = 0x52，寻感应区内所有符合14443A标准的卡
 *                     = 0x26，寻未进入休眠状态的卡
 *         pTagType，卡片类型代码
 *                   = 0x4400，Mifare_UltraLight
 *                   = 0x0400，Mifare_One(S50)
 *                   = 0x0200，Mifare_One(S70)
 *                   = 0x0800，Mifare_Pro(X))
 *                   = 0x4403，Mifare_DESFire
 * 返回  : 状态值
 *         = MI_OK，成功
 */
char PcdRequest(uint8_t req_code, uint8_t *pTagType)
{
    char status;
    uint32_t unLen;
    uint8_t ucComMF522Buf[MAXRLEN];

    ClearBitMask(Status2Reg, 0x08);  //清除RC522寄存位,清理指示MIFARECyptol单元接通以及所有卡的数据通信被加密的情况
    WriteRawRC(BitFramingReg, 0x07); //写RC522寄存器，发送的最后一个字节的七位
    SetBitMask(TxControlReg, 0x03);  //写RC522寄存位，TX1,TX2管脚的输出信号传递经发送调制的13.56的能量载波信号

    ucComMF522Buf[0] = req_code;//存入寻卡方式

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 1, ucComMF522Buf, &unLen);
    if ((status == MI_OK) && (unLen == 0x10))//寻卡成功返回卡类型
    {
        *pTagType = ucComMF522Buf[0];
        *(pTagType + 1) = ucComMF522Buf[1];
    }
    else
    {
        status = MI_ERR;
    }
    return status;
}
 

 
/*
 * 函数名：PcdAnticoll
 * 描述  ：防冲撞
 * 输入  ：pSnr，卡片序列号，4字节
 * 返回  : 状态值
 *         = MI_OK，成功
 */
char PcdAnticoll(uint8_t *pSnr)
{
    char status;
    uint8_t i, snr_check = 0;
    uint32_t unLen;
    uint8_t ucComMF522Buf[MAXRLEN];
    ClearBitMask(Status2Reg, 0x08);//清MFCryptol On位,只有成功执行MFAuthent命令后，该位才能置位
    WriteRawRC(BitFramingReg, 0x00);//清理寄存器,停止收发
    ClearBitMask(CollReg, 0x80);//清ValuesAfterColl所有接收的位在冲突后被清除
    ucComMF522Buf[0] = PICC_ANTICOLL1;//卡片防冲突命令
    ucComMF522Buf[1] = 0x20;

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 2, ucComMF522Buf, &unLen);
    if (status == MI_OK)
    {
        for (i = 0; i < 4; i++)
        {
            *(pSnr + i) = ucComMF522Buf[i];
            snr_check ^= ucComMF522Buf[i];
        }
        if (snr_check != ucComMF522Buf[i])
        {
            status = MI_ERR;
        }
    }
    SetBitMask(CollReg, 0x80);
    return status;
}
 

  
/*
 * 函数名：PcdSelect
 * 描述  ：选定卡片
 * 输入  ：pSnr，卡片序列号，4字节
 * 返回  : 状态值
 *         = MI_OK，成功
 */
char PcdSelect(uint8_t *pSnr)
{
    char status;
    uint8_t i;
    uint32_t unLen;
    uint8_t ucComMF522Buf[MAXRLEN];
    ucComMF522Buf[0] = PICC_ANTICOLL1;//防冲撞
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;
    for (i = 0; i < 4; i++)
    {
        ucComMF522Buf[i + 2] = *(pSnr + i);
        ucComMF522Buf[6] ^= *(pSnr + i);
    }
    CalulateCRC(ucComMF522Buf, 7, &ucComMF522Buf[7]);
    ClearBitMask(Status2Reg, 0x08);
    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 9, ucComMF522Buf, &unLen);
    if ((status == MI_OK) && (unLen == 0x18))
    {
        status = MI_OK;
    }
    else
    {
        status = MI_ERR;
    }
    return status;
}






/*
 * 函数名：PcdAuthState
 * 描述  ：验证卡片密码
 * 输入  ：ucAuth_mode，密码验证模式
 *                     = 0x60，验证A密钥
 *                     = 0x61，验证B密钥
 *         uint8_t ucAddr，块地址
 *         pKey，密码
 *         pSnr，卡片序列号，4字节
 * 返回  : 状态值
 *         = MI_OK，成功
 */
char PcdAuthState(uint8_t auth_mode, uint8_t addr, uint8_t *pKey, uint8_t *pSnr)
{
    char status;
    uint32_t unLen;
    uint8_t i, ucComMF522Buf[MAXRLEN];
    ucComMF522Buf[0] = auth_mode;
    ucComMF522Buf[1] = addr;
    for (i = 0; i < 6; i++)
    {
        ucComMF522Buf[i + 2] = *(pKey + i);
    }
    for (i = 0; i < 6; i++)
    {
        ucComMF522Buf[i + 8] = *(pSnr + i);
    }
    status = PcdComMF522(PCD_AUTHENT, ucComMF522Buf, 12, ucComMF522Buf, &unLen);
    if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08)))
    {
        status = MI_ERR;
    }
    return status;
}




/*
 * 函数名：PcdRead
 * 描述  ：读取M1卡一块数据
 * 输入  ：uint8_t ucAddr，块地址
 *         pData，读出的数据，16字节
 * 返回  : 状态值
 *         = MI_OK，成功
 * 调用  ：外部调用
 */
char PcdRead(uint8_t addr, uint8_t *pData)
{
    char status;
    uint32_t unLen;
    uint8_t i, ucComMF522Buf[MAXRLEN];
    ucComMF522Buf[0] = PICC_READ;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);
    if ((status == MI_OK) && (unLen == 0x90))
    {
        for (i = 0; i < 16; i++)
        {
            *(pData + i) = ucComMF522Buf[i];
        }
    }
    else
    {
        status = MI_ERR;
    }
    return status;
}





/*
 * 函数名：PcdWrite
 * 描述  ：写数据到M1卡一块
 * 输入  ：uint8_t ucAddr，块地址
 *         pData，写入的数据，16字节
 * 返回  : 状态值
 *         = MI_OK，成功
 * 调用  ：外部调用
 */
char PcdWrite(uint8_t addr, uint8_t *pData)
{
    char status;
    uint32_t unLen;
    uint8_t i, ucComMF522Buf[MAXRLEN];
    ucComMF522Buf[0] = PICC_WRITE;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);
    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);
    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {
        status = MI_ERR;
    }
    if (status == MI_OK)
    {
        for (i = 0; i < 16; i++)
        {
            ucComMF522Buf[i] = *(pData + i);
        }
        CalulateCRC(ucComMF522Buf, 16, &ucComMF522Buf[16]);

        status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 18, ucComMF522Buf, &unLen);
        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {
            status = MI_ERR;
        }
    }
    return status;
}





/*
 * 函数名：PcdValue
 * 描述  ：扣款和充值
 * 输入  ：dd_mode[IN]：命令字
 *              0xC0 = 扣款
 *              0xC1 = 充值
 *         addr[IN]：钱包地址
 *         pValue[IN]：4字节增(减)值，低位在前
 * 返回  : 状态值
 *         = MI_OK，成功
 */
char PcdValue(uint8_t dd_mode, uint8_t addr, uint8_t *pValue)
{
    char status;
    uint32_t unLen;
    uint8_t i, ucComMF522Buf[MAXRLEN];
    ucComMF522Buf[0] = dd_mode;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);
    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {
        status = MI_ERR;
    }
    if (status == MI_OK)
    {
        for (i = 0; i < 16; i++)
        {
            ucComMF522Buf[i] = *(pValue + i);
        }
        CalulateCRC(ucComMF522Buf, 4, &ucComMF522Buf[4]);
        unLen = 0;
        status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 6, ucComMF522Buf, &unLen);
        if (status != MI_ERR)
        {
            status = MI_OK;
        }
    }
    if (status == MI_OK)
    {
        ucComMF522Buf[0] = PICC_TRANSFER;
        ucComMF522Buf[1] = addr;
        CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

        status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {
            status = MI_ERR;
        }
    }
    return status;
}





/*
 * 函数名：PcdBakValue
 * 描述  ：备份钱包
 * 输入  ：sourceaddr[IN]：源地址
 *         goaladdr[IN]：目标地址
 * 返回  : 状态值
 *         = MI_OK，成功
 */
char PcdBakValue(uint8_t sourceaddr, uint8_t goaladdr)
{
    char status;
    uint32_t unLen;
    uint8_t ucComMF522Buf[MAXRLEN];
    ucComMF522Buf[0] = PICC_RESTORE;
    ucComMF522Buf[1] = sourceaddr;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);
    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);
    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {
        status = MI_ERR;
    }
    if (status == MI_OK)
    {
        ucComMF522Buf[0] = 0;
        ucComMF522Buf[1] = 0;
        ucComMF522Buf[2] = 0;
        ucComMF522Buf[3] = 0;
        CalulateCRC(ucComMF522Buf, 4, &ucComMF522Buf[4]);
        status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 6, ucComMF522Buf, &unLen);
        if (status != MI_ERR)
        {
            status = MI_OK;
        }
    }
    if (status != MI_OK)
    {
        return MI_ERR;
    }
    ucComMF522Buf[0] = PICC_TRANSFER;
    ucComMF522Buf[1] = goaladdr;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);
    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);
    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {
        status = MI_ERR;
    }
    return status;
}




/*
 * 函数名：PcdHalt
 * 描述  ：命令卡片进入休眠状态
 * 返回  : 状态值
 *         = MI_OK，成功
 */
char PcdHalt(void)
{
    uint32_t unLen;
    uint8_t ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_HALT;
    ucComMF522Buf[1] = 0;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    return MI_OK;
}




/*
 * 函数名：CalulateCRC
 * 描述  ：用RC522计算CRC16
 * 输入  ：pIndata，计算CRC16的数组
 *         ucLen，计算CRC16的数组字节长度
 *         pOutData，存放计算结果存放的首地址
 * 返回  : 无
 * 调用  ：内部调用
 */
void CalulateCRC(uint8_t *pIndata, uint8_t len, uint8_t *pOutData)
{
    uint8_t i, n;
    ClearBitMask(DivIrqReg, 0x04);
    WriteRawRC(CommandReg, PCD_IDLE);
    SetBitMask(FIFOLevelReg, 0x80);
    for (i = 0; i < len; i++)
    {
        WriteRawRC(FIFODataReg, *(pIndata + i));
    }
    WriteRawRC(CommandReg, PCD_CALCCRC);
    i = 0xFF;
    do
    {
        n = ReadRawRC(DivIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x04));
    pOutData[0] = ReadRawRC(CRCResultRegL);
    pOutData[1] = ReadRawRC(CRCResultRegM);
}



/*
 * 函数名：PcdRese
 * 描述  ：复位RC522 
 * 返回  : 状态值
 *         = MI_OK，成功
 */
char PcdReset(void)
{
	RC522_Reset_Disable();
    delay_ms(10);
    RC522_Reset_Enable();
    delay_ms(60);
    RC522_Reset_Disable();
    delay_ms(500);
    WriteRawRC(CommandReg, PCD_RESETPHASE);//复位指令
    delay_ms(2);

    WriteRawRC(ModeReg, 0x3D);//定义发送和接收常用模式,和Mifare卡通讯，CRC初始值0x6363
    WriteRawRC(TReloadRegL, 30);//16位定时器低位
    WriteRawRC(TReloadRegH, 0);//16位定时器高位
    WriteRawRC(TModeReg, 0x8D);//定义内部定时器的设置
    WriteRawRC(TPrescalerReg, 0x3E);//设置定时器分频系数
    WriteRawRC(TxAutoReg, 0x40);//调制发送信号为100%ASK

    ClearBitMask(TestPinEnReg, 0x80);
    WriteRawRC(TxAutoReg, 0x40);

    return MI_OK;
}

 
/*
 * 函数名：ReadRawRC
 * 描述  ：读RC522寄存器
 * 输入  ：ucAddress，寄存器地址
 * 返回  : 寄存器的当前值
 */
uint8_t ReadRawRC(uint8_t Address)
{
    uint8_t ucAddr;
    uint8_t ucResult = 0;
    ucAddr = ((Address << 1) & 0x7E) | 0x80;//最高位置1，读操作;最低为默认0
    delay_ms(1);
    RC522_CS_Enable();
    SPI_WriteNBytes(SPI1_SPI, &ucAddr, 1);  //向总线写多个数据
    SPI_ReadNBytes(SPI1_SPI, &ucResult, 1); //向总线读多个数据
    RC522_CS_Disable();
    return ucResult;
}
 
 
/*
 * 函数名：WriteRawRC
 * 描述  ：写RC522寄存器
 * 输入  ：ucAddress，寄存器地址
 *         ucValue，写入寄存器的值
 */
void WriteRawRC(uint8_t Address, uint8_t value)
{
    uint8_t ucAddr;
    uint8_t write_buffer[2] = {0};
    ucAddr = ((Address << 1) & 0x7E);//最高位置0，写操作;最低为默认0
    write_buffer[0] = ucAddr;
    write_buffer[1] = value;
    delay_ms(1);
    RC522_CS_Enable();
    SPI_WriteNBytes(SPI1_SPI, write_buffer, 2);
    RC522_CS_Disable();
}
 
 
/*
 * 函数名：SetBitMask
 * 描述  ：对RC522寄存器置位，可对多个位同时操作
 * 输入  ：ucReg，寄存器地址
 *         ucMask，置位值,例0x13，0001 0011 
 */
void SetBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t temp = 0x00;

    temp = ReadRawRC(reg);        //读寄存器
    WriteRawRC(reg, temp | mask); //将目标位置1 
}
 
 
/*
 * 函数名：ClearBitMask
 * 描述  ：对RC522寄存器清位
 * 输入  ：ucReg，寄存器地址
 *         ucMask，清位值
 */
void ClearBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t temp = 0x00;

    temp = ReadRawRC(reg);
    WriteRawRC(reg, temp & ~mask);
}




/*
 * 函数名：PcdAntennaOn
 * 描述  ：开启天线 
 * TxControlReg寄存器位0置1，引脚TX1上的输出信号将传输由传输数据调制的13.56 MHz能量载波。
 * TxControlReg寄存器位1置1，引脚TX2上的输出信号将发送由传输数据调制的13.56 MHz能量载波。
 */
void PcdAntennaOn(void)
{
    uint8_t i;
    i = ReadRawRC(TxControlReg);
    if (!(i & 0x03))
    {
        SetBitMask(TxControlReg, 0x03);
    }
}
 
 
/*
 * 函数名：PcdAntennaOff
 * 描述  ：关闭天线 
 */
void PcdAntennaOff(void)
{
    ClearBitMask(TxControlReg, 0x03);
}
 



/*
 * 函数名：RC522_PcdConfig_Type
 * 描述  ：设置RC522的工作方式
 * 工作方式ISO14443_A
 * 复位中已经设置过，该函数暂时未调用
 */
void RC522_Config(uint8_t Card_Type)
{
    ClearBitMask(Status2Reg, 0x08);
    WriteRawRC(ModeReg, 0x3D);
    WriteRawRC(RxSelReg, 0x86);
    WriteRawRC(RFCfgReg, 0x7F);
    WriteRawRC(TReloadRegL, 30);
    WriteRawRC(TReloadRegH, 0);
    WriteRawRC(TModeReg, 0x8D);
    WriteRawRC(TPrescalerReg, 0x3E);
    delay_ms(5);
    PcdAntennaOn();
}


