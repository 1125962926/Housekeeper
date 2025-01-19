#include "./BSP/ESP8266/8266wifi.h"
#include "./SYSTEM/delay/delay.h"
#include "string.h"

//约束在数据末尾加上0x0d，0x0a表示数据结束（接收时不会加入数据中）
/*模块使用流程：

1、初始化（选择模式）
2、如果是透传则打开透传
3、发送数据或接收数据
4、如果是透传则关闭透传
5、关闭连接

*/


/*********************************************************************************************/

//定义一个工作模式的全局变量，方便函数调用时自动选择
/*   1为单连接TCP，esp做客户端;
 *   2为UDP传输;
 *   3为TCP透传;
 *   4为UDP透传
 *   5为多连接TCP，esp做服务器;
 */
static uint8_t esp8266wifi_working_mode=0;

/*********************************************************************************************/
/*底层函数*/




/**
 * @brief       检测应答指令
 * @param       *str: 要验证的预期结果
 * @retval      *strx：与预期相同返回原字符串，不同则返回NULL
 */
uint8_t* esp8266wifi_check_cmd(uint8_t *str_target,char *str_rx)
{

    str_rx=strstr((const char*)str_rx,(const char*)str_target);
	
    return (uint8_t *)str_rx;
}




/**
 * @brief       恢复出厂设置
 * @param       无
 * @retval      无
 */
void esp8266wifi_reset(void)
{
    printf("正在复位...\n");
	esp8266wifi_send_cmd("AT+RESTORE",0,normal_waittime);
    delay_ms(1000);
    printf("复位完成\n");
}




/**
 * @brief       设置传输模式
 * @param       mode: 0为普通传输模式
  *                  1为透传模式，仅⽀持 TCP 单连接和 UDP 固定通信对端的情况
 * @retval      无
 */
void esp8266wifi_set_trans_mode(uint8_t mode)
{
    switch (mode)
    {
        case 0:
            esp8266wifi_send_cmd("AT+CIPMODE=0",0,normal_waittime);
            break;
        case 1:
            esp8266wifi_send_cmd("AT+CIPMODE=1",0,normal_waittime);
            break;
        default:
            break;
    }
}



/**
 * @brief       设置Wi-Fi模式
 * @param       mode: 1为Station 模式，客户端
 *                    2为SoftAP 模式，开启热点
 *                    3为SoftAP+Station 双模式
 * @retval      无
 */
void esp8266wifi_set_wifi_mode(uint8_t mode)
{
    switch (mode)
    {
        case 1:
            esp8266wifi_send_cmd("AT+CWMODE=1",0,normal_waittime);
            break;
        case 2:
            esp8266wifi_send_cmd("AT+CWMODE=2",0,normal_waittime);
            break;
        case 3:
            esp8266wifi_send_cmd("AT+CWMODE=3",0,normal_waittime);
            break;
        default:
            break;
    }
}


/**
 * @brief       向esp8266wifi发送指令,自动加上回车换行
 * @param       *cmd: 要发送的指令
 *              *expect: 期望指令，检查模块返回值，查看指令执行情况,若不检查，调用时给0（不带引号）
 *              waittime: 等待数据接收时常(ms)，等待模块回应的最大时间
 * @retval      state：应答正确返回1，0则失败
 */
uint8_t esp8266wifi_send_cmd(char *cmd,uint8_t *expect,int waittime)
{
	uint8_t state=0;
    int waittime_old=waittime;
    char esp8_rx_data[MAX_temp_array];
    char temp[MAX_temp_array];         
	
    esp8266wifi_rx_sta=0;  
    sprintf(temp,"%s\r\n",cmd);
	esp8266wifi_send_string(temp);
    while ((esp8266wifi_USART->SR & 0X40) == 0);    
	if(waittime)		
	{
		while(--waittime)	
		{
			delay_ms(1);
			if(esp8266wifi_rx_sta&0X8000)
			{
                waittime=waittime_old;                
                sprintf(esp8_rx_data,"%s",(char *)esp8266wifi_rx_buf);
                // printf("模块返回:%s\n",esp8_rx_data);
                if(expect)
                {
                    if(esp8266wifi_check_cmd(expect,esp8_rx_data))
                    {
                        state=esp8266_OK;
                        printf("模块应答正确: %s\n",expect);                                                                  
                    }                   
                }				
				esp8266wifi_rx_sta=0;//清空接收完成标志位
			} 
		} 
	}
    
	return state;
} 





/**
 * @brief       关闭当前的连接
 * @param       ID：多连接关闭的ID,需要关闭的连接 ID 号。当 ID 为 5 时，关闭所有连接。
 *                  （开启 server 后 ID 为 5 ⽆效）   
 */
void esp8266wifi_close_connect(uint8_t ID)
{
    uint8_t num=esp8266wifi_working_mode;//获取当前的工作模式
    if(num==1)//关闭单连接
    {
        esp8266wifi_send_cmd("AT+CIPCLOSE",0,MAX_waittime);//关闭单连接
    }
    else if(num==2)//关闭多连接
    {
        char str1[20];
        sprintf(str1,"AT+CIPCLOSE=%d",ID);
        esp8266wifi_send_cmd(str1,0,MAX_waittime);
    }
    
}





/**
 * @brief       esp8266wifi模块初始化
 * @param       RUNmode：1为单连接TCP，esp做客户端;
 *                       2为UDP传输;
 *                       3为TCP透传;
 *                       4为UDP透传;
 *                       5为多连接TCP，esp做服务器;
 * @param       wifi_mode: 1为Station 模式，连接外部wifi
 *                         2为SoftAP 模式，开启热点
 *                         3为SoftAP+Station 双模式
 * @note        检测IP是否获得：AT+CIFSR
 *              若要更换模块使用方式，重新调用初始化即可
 *              模块只有在RUNmode为模式5开启服务器后才能作为服务器，其余均为Station客户端
 *              
 * @retval      0为错误，1为初始化完成
 */
uint8_t esp8266_Init(uint8_t RUNmode,uint8_t wifi_mode)
{  
	uint8_t ERROR_count=0;//ERROR_count用于计数错误时的尝试次数
    uint8_t esp_state=esp8266_OK;//获取当前的工作模式
    char temp1[MAX_temp_array];//用于字符串连接
    
    esp8266wifi_working_mode=RUNmode;//保存全局工作模式
    printf("\r\nESP8266wifi模块正在初始化\r\n");
    esp8266wifi_reset();//复位模块       

	esp8266wifi_send_cmd("ATE0",0,normal_waittime);//关闭回显
    esp8266wifi_send_cmd("AT+CIPDINFO=0",0,normal_waittime);//关闭显示IP和端口

    esp8266wifi_set_wifi_mode(wifi_mode);//设置wifi模式
    if(wifi_mode==wifi_mode_Station)//连接WIFI
    {
        sprintf(temp1,"AT+CWJAP=\"%s\",\"%s\"",SoftAP_WIFI_SSID,SoftAP_WIFI_PASSWORD);
        ERROR_count=0;
        printf("\r\n正在连接WIFI...\r\n");
        do//连接WIFI
        {            
            esp_state=esp8266wifi_send_cmd(temp1,"OK",MAX_waittime);
            if(esp_state==esp8266_ERROR)
            {               
                ERROR_count++;
                delay_ms(1000);//延时1s
                if(ERROR_count==esp8266_Init_Try_Time)
                {    
                    printf("\r\n连接WIFI失败\r\n");
                    return esp8266_ERROR;
                }
            }
        }while(esp_state!=esp8266_OK);
        printf("\r\nWIFI连接成功\r\n");
    }
    else//配置WIFI信息
    {
        sprintf(temp1,"AT+CWSAP=\"%s\",\"%s\",%d,%d",
        esp8266wifi_WIFI_SSID,esp8266wifi_WIFI_PASSWORD,esp8266wifi_WIFI_channal,esp8266wifi_WIFI_encoding);
        ERROR_count=0;
        printf("\r\n正在配置WIFI...\r\n");
        do//设置WIFI信息
        {            
            esp_state=esp8266wifi_send_cmd(temp1,"OK",MAX_waittime);
            if(esp_state==esp8266_ERROR)
            {
                ERROR_count++;
                delay_ms(1000);
                if(ERROR_count==esp8266_Init_Try_Time)
                {         
                    printf("\r\n配置WIFI失败\r\n");              
                    return esp8266_ERROR;
                }
            }
        }while(esp_state!=esp8266_OK);
        printf("\r\n设置WIFI成功\r\n");
        
    }
    if(RUNmode==mode_1_TCP_Client)//单连接TCP,esp做Station客户端
    {       
        esp8266wifi_set_trans_mode(0);//设置为普通传输模式，非透传
        esp8266wifi_send_cmd("AT+CIPMUX=0",0,normal_waittime);//单连接模式                    
        sprintf(temp1,"AT+CIPSTART=\"TCP\",\"%s\",%s",esp8266wifi_TCP_IP,esp8266wifi_TCP_PORT);
        ERROR_count=0;
        printf("\r\n正在连接服务器...\r\n");
        do//esp作为客户端连接到服务器
        {
            esp_state=esp8266wifi_send_cmd(temp1,"CONNECTED",MAX_waittime);
            if(esp_state==esp8266_ERROR)
            {                
                ERROR_count++;
                delay_ms(1000);
                if(ERROR_count==esp8266_Init_Try_Time)
                {          
                    printf("\r\n连接到服务器失败\r\n");             
                    return esp8266_ERROR;
                }
            }
        }while(esp_state!=esp8266_OK);
        printf("\r\n成功连接到服务器\r\n");
    }
    else if(RUNmode==mode_2_UDP)//UDP,该模式下远端固定时，发送数据AT+CIPSEND=4,7要指定ID，断开连接时AT+CIPCLOSE=4也要指定ID；
    {
        esp8266wifi_set_trans_mode(0);//设置为普通传输模式，非透传              
        if(esp8266wifi_UDP_mode==0)//远端固定的UDP通信,要开启多连接
        {
            esp8266wifi_send_cmd("AT+CIPMUX=1",0,normal_waittime);//多连接模式
            //分配连接号,UDP传输，远端IP，远端端口，本地端口，端口保持
            sprintf(temp1,"AT+CIPSTART=%d,\"UDP\",\"%s\",%s,%s,%d",
            esp8266wifi_UDP_Link_ID,esp8266wifi_UDP_IP,esp8266wifi_UDP_SoftAP_PORT,esp8266wifi_UDP_Local_PORT,esp8266wifi_UDP_mode);            
        }
        else if(esp8266wifi_UDP_mode==2)//远端可变
        {
            esp8266wifi_send_cmd("AT+CIPMUX=0",0,normal_waittime);//单连接模式
            //UDP传输，远端IP，远端端口，本地端口，端口保持
            sprintf(temp1,"AT+CIPSTART=\"UDP\",\"%s\",%s,%s,%d",
            esp8266wifi_UDP_IP,esp8266wifi_UDP_SoftAP_PORT,esp8266wifi_UDP_Local_PORT,esp8266wifi_UDP_mode);
           
        }  
        ERROR_count=0;
        printf("\r\n正在连接服务器...\r\n");
        do//esp作为客户端连接到服务器
        {
            esp_state=esp8266wifi_send_cmd(temp1,"CONNECTED",MAX_waittime);
            if(esp_state==esp8266_ERROR)
            {                
                ERROR_count++;
                delay_ms(1000);
                if(ERROR_count==esp8266_Init_Try_Time)
                {    
                    printf("\r\n连接到服务器失败\r\n");                   
                    return esp8266_ERROR;
                }
            }
        }while(esp_state!=esp8266_OK);
        printf("\r\n成功连接到服务器\r\n");        
    }
    else if(RUNmode==mode_3_TCP_transparent)//TCP单连接透传
    {
        esp8266wifi_send_cmd("AT+CIPMUX=0",0,normal_waittime);//单连接模式，才能开启透传
        sprintf(temp1,"AT+CIPSTART=\"TCP\",\"%s\",%s",esp8266wifi_TCP_IP,esp8266wifi_TCP_PORT);                 
        ERROR_count=0;
        printf("\r\n正在连接服务器...\r\n");
        do//esp作为客户端连接到服务器
        {
            esp_state=esp8266wifi_send_cmd(temp1,"CONNECTED",MAX_waittime);
            if(esp_state==esp8266_ERROR)
            {                
                ERROR_count++;
                delay_ms(1000);
                if(ERROR_count==esp8266_Init_Try_Time)
                {   
                    printf("\r\n连接到服务器失败\r\n");                    
                    return esp8266_ERROR;
                }
            }
        }while(esp_state!=esp8266_OK);
        printf("\r\n成功连接到服务器\r\n");    
        esp8266wifi_set_trans_mode(1);//设置透传模式
    }
    else if(RUNmode==mode_4_UDP_transparent)//UDP透传
    {
        esp8266wifi_send_cmd("AT+CIPMUX=0",0,normal_waittime);//单连接模式，才能开启透传
        sprintf(temp1,"AT+CIPSTART=\"UDP\",\"%s\",%s,%s,%d",
        esp8266wifi_UDP_IP,esp8266wifi_UDP_SoftAP_PORT,esp8266wifi_UDP_Local_PORT,esp8266wifi_UDP_mode);
        ERROR_count=0;
        printf("\r\n正在连接服务器...\r\n");
        do//esp作为客户端连接到服务器
        {
            esp_state=esp8266wifi_send_cmd(temp1,"CONNECTED",MAX_waittime);
            if(esp_state==esp8266_ERROR)
            {                
                ERROR_count++;
                delay_ms(1000);
                if(ERROR_count==esp8266_Init_Try_Time)
                {        
                    printf("\r\n连接到服务器失败\r\n");               
                    return esp8266_ERROR;
                }
            }
        }while(esp_state!=esp8266_OK);
        esp8266wifi_set_trans_mode(1);//设置透传模式
    }
    else if(RUNmode==mode_5_TCP_Server)//多连接TCP，esp做服务器
    {
        /*ESP8266 作为 TCP server 有超时机制，如果连接建⽴后，⼀段时间内⽆数据来往，ESP8266 TCP server 会将 TCP client 踢掉。
        因此，请在 PC TCP client 连上 ESP8266 TCP server 后建⽴⼀个 2s 的循环数据发送，⽤于保持连接。*/
        esp8266wifi_set_trans_mode(0);//设置为普通传输模式，非透传
        esp8266wifi_send_cmd("AT+CIPMUX=1",0,normal_waittime);//多连接模式
        sprintf(temp1,"AT+CIPAP=\"%s\"",esp8266wifi_SoftAP_IP);
        esp8266wifi_send_cmd(temp1,0,normal_waittime);//设置SoftAP模式IP
        sprintf(temp1,"AT+CIPSERVER=1,%s",esp8266wifi_SoftAP_PORT);
        esp8266wifi_send_cmd(temp1,0,normal_waittime);//开启服务器,设置端口
    }
    return esp8266_OK;
}





/***************************************************************************************/
/*esp8266wifi模块发出数据*/


//模式1
/**
 * @brief       单连接TCP，esp发送数据到主机
 * @param       *data: 要发送的数据
 * @retval      state：返回1发送成功，0则失败
 */
uint8_t mode_1_send_date(char *data)
{
    uint8_t state = esp8266_OK;//发送成功状态
    char str1[20];

    sprintf(str1,"AT+CIPSEND=%d",strlen(data)+2);//预留回车换行长度，在发送函数中自动加上
    /*实测不加回车换行也能发送和接收数据，但是如果要在串口显示和接收（串口接收到0x0d和0x0a为结束），就要加上，同时也方便数据管理*/
    esp8266wifi_send_cmd(str1,">",normal_waittime);//发送数据长度
    state=esp8266wifi_send_cmd(data,"SEND OK",MAX_waittime);//发送数据
    return state;
}


//模式2
/**
 * @brief       UDP，esp发送数据到远端
 * @param       *data: 要发送的数据
 * @retval      state：返回1发送成功，0则失败
 */
uint8_t mode_2_send_date(char *Data)
{
    uint8_t state = esp8266_OK;//发送成功状态
    char str1[MAX_temp_array];  

    //发送数据
    if(esp8266wifi_UDP_mode==0)//远端固定
    {
        sprintf(str1,"AT+CIPSEND=%d,%d",esp8266wifi_UDP_Link_ID,strlen(Data)+2);
    }
    else if(esp8266wifi_UDP_mode==2)//远端可变
    {
        sprintf(str1,"AT+CIPSEND=%d",strlen(Data)+2);
    }
    
    esp8266wifi_send_cmd(str1,">",normal_waittime);
    state=esp8266wifi_send_cmd(Data,"SEND OK",MAX_waittime);
    return state;
}
    

/**
 * @brief       UDP，esp发送数据到指定远端
 * @param       *data: 要发送的数据
 * @param       SendIP: 要发送到的IP
 * @param       SendPort: 要发送到的端口
 * @retval      state：返回1发送成功，0则失败
 */    
uint8_t mode_2_send_date_IP(char *Data,char *SendIP,uint16_t SendPort)
{
    uint8_t state = esp8266_OK;//发送成功状态
    char str1[MAX_temp_array];  

    //发送数据
    sprintf(str1,"AT+CIPSEND=%d,\"%s\",%d",strlen(Data)+2,SendIP,SendPort);
    
    state=esp8266wifi_send_cmd(str1,">",MAX_waittime);
    state=esp8266wifi_send_cmd(Data,"SEND OK",MAX_waittime);
    return state;
}


//模式3、4

/**
 * @brief       透传transparent transmission,TCP和UDP共用
 * @param       *data: 要发送的数据
 * @note        传输数据前要调用开始透传，结束后退出透传,中途可以直接发送数据
 * @retval      无
 */
void mode_3_and_4_send_date(char *data)
{
    esp8266wifi_send_string(data);
}



//模式5
/**
 * @brief       多连接TCP,esp作为Server服务器
 * @param       *data: 要发送的数据
 * @param       Link_ID: 要收到数据的ID号
 * @retval      state：返回1发送成功，0则失败
 */
uint8_t mode_5_send_date(char *Data,uint8_t Link_ID)
{
    uint8_t state = esp8266_OK;//发送成功状态
    char str1[MAX_temp_array];  

    sprintf(str1,"AT+CIPSEND=%d,%d",Link_ID,strlen(Data)+2);
    esp8266wifi_send_cmd(str1,">",normal_waittime);
    state=esp8266wifi_send_cmd(Data,"SEND OK",MAX_waittime);
    return state;
}




/*********************************************************************************************/
/*用户调用函数*/


/**
 * @brief       开启透传发送
 * @param       无
 * @note        仅⽀持 TCP 单连接和 UDP 固定通信对端的情况
 * @retval      state：返回1成功，0则失败
 */
void esp8266_start_trans(void)
{
	esp8266wifi_send_cmd("AT+CIPSEND",">",MAX_waittime);
}



/**
 * @brief       退出透传发送
 * @param       无
 * @retval      result：返回1发送成功，0则失败
 */
uint8_t esp8266_quit_trans(void)
{
	uint8_t result=esp8266_OK;
	esp8266wifi_send_string("+++");
	delay_ms(1000);					
	result=esp8266wifi_send_cmd("AT","OK",MAX_waittime);
	if(!result)
		printf("quit_trans failed!");
	else
		printf("quit_trans success!");
	return result;
}



/**
 * @brief       通过串口向esp8266模块发送数据
 * @param       *data：要发送的数据
 * @param       ID：要收到数据的ID号
 * @retval      result：返回1发送成功，0则失败
 */
uint8_t esp8266_send_data(char *data,uint8_t ID)
{
	uint8_t result=esp8266_OK;
    switch (esp8266wifi_working_mode)
    {
        case 1:
            result = mode_1_send_date(data);
            break;
        case 2:
            result = mode_2_send_date(data);
            break;
        case 3:                        
        case 4:
            mode_3_and_4_send_date(data);           
            break;
        case 5:
            result = mode_5_send_date(data,ID);           
            break;
        default:
            break;
    }
	return result;
}





/**
 * @brief       接收数据处理，对esp8266返回的数据进行解码,并清除串口接收完成标志位
 * @param       *true_data：接收到的有效数据
 * @note        以下是esp8266返回数据的HEX显示与16进制显示,串口接收数据时，已在串口接收中断中滤除开头的0D 0A
 * 
 *              +IPD,4:qq
 *              0D 0A      2B 49 50 44 2C(,) 34 3A(:) 71 71      0D 0A
 *
 *              +IPD,0,4:qq
 *              0D 0A      2B 49 50 44 2C(,) 30 2C(,) 34 3A(:) 71 71       0D 0A 
 * 
 * @retval      ID:发送数据的ID
 */
uint8_t esp8266_solve_receive_data(char *true_data,uint8_t old_date[])
{
    uint8_t ID = 0;//多连接模式的ID号
    uint8_t date_byte = 5;//默认从第一个逗号后一位开始，逗号以前的“+IPD”和逗号都不要（+IPD,）
    uint8_t j, mode = esp8266wifi_working_mode;
    char temp[MAX_temp_array];

    if((mode==mode_3_TCP_transparent)||(mode==mode_4_UDP_transparent))
    {
        sprintf(true_data,"%s",(char *)old_date);
    }
    else
    {
        if(mode==mode_5_TCP_Server)//多连接模式
        {
            ID = old_date[5];
        }  
        while(old_date[date_byte]!=0x3A)//冒号(:)
        {           
            date_byte++;
        }
        date_byte++;//跳过冒号(:)
        while(old_date[date_byte]!='\0')
        {           
            temp[j] = old_date[date_byte];
            date_byte++;
            j++;
        }    
        temp[j] = '\0';//结束符
        sprintf(true_data,"%s",temp);
    }                   
    return ID;
}


