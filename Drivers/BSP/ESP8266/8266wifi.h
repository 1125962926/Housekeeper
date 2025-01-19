#ifndef _8266wifi_H
#define _8266wifi_H


#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart2/usart2.h"


/******************************************************************************************/
/* 串口更改，可以改为任意配置好的USART */
/*串口，接收完成标志位，接收缓冲，串口发送函数*/
#define esp8266wifi_USART               USART2_UX
#define esp8266wifi_rx_sta              g_usart2_rx_sta
#define esp8266wifi_rx_buf              g_usart2_rx_buf
#define esp8266wifi_send_string         usart2_send_string


/******************************************************************************************/
/*路由器wifi 信息*/
/*名称，密码*/
#define SoftAP_WIFI_SSID           "92119211"
#define SoftAP_WIFI_PASSWORD       "92119211"



/*esp8266作为服务器wifi 信息*/    
/*名称，密码，通道，加密方式*/

/*加密方式
0：OPEN
2：WPA_PSK
3：WPA2_PSK
4：WPA_WPA2_PSK
*/
#define esp8266wifi_WIFI_SSID           "NiDie"
#define esp8266wifi_WIFI_PASSWORD       "12345678"
#define esp8266wifi_WIFI_channal        1
#define esp8266wifi_WIFI_encoding       4

/******************************************************************************************/
/*TCP传输信息*/ 

/*esp作为SoftAP服务器的IP和端口信息*/    
#define esp8266wifi_SoftAP_IP           "192.168.4.1"
#define esp8266wifi_SoftAP_PORT         "8080"


/*esp作为Station客户端，要接入的服务器IP和端口信息*/ 
#define esp8266wifi_TCP_IP          "192.168.1.111"
#define esp8266wifi_TCP_PORT        "8081"



/******************************************************************************************/
/*UDP传输信息*/
/*UDP远端和TCP的服务器使用同一组IP和端口，方便调试*/
/*IP，远程端口，连接ID，本地端口，端口保持*/ 
#define esp8266wifi_UDP_IP              "192.168.1.113"
#define esp8266wifi_UDP_SoftAP_PORT     "8081"
#define esp8266wifi_UDP_Link_ID          4
#define esp8266wifi_UDP_Local_PORT       "1112"


#define esp8266wifi_UDP_mode             2
/*
‣ 0：收到数据后，不更改远端⽬标，默认值为 0
‣ 1：收到数据后，改变⼀次远端⽬标
‣ 2：收到数据后，改变远端⽬标
*/
/*0 表示当前 UDP 传输建⽴后，UDP 远端不会被其他设备更改；即使有其他设备通过 UDP 协议发数据到
ESP8266 UDP 端⼝ 1112，ESP8266 的第 4 号 UDP 传输的远端也不会被替换，使⽤指令 
“AT+CIPSEND=4, X” 发送数据，仍然是当前固定的 PC 端收到。*/
/*2 表示当前 UDP 传输建⽴后，UDP 传输远端仍然会更改；UDP 传输远端会⾃动更改为最近⼀个与
ESP8266 UDP 通信的远端。*/


/*********************************************************************************************/
/*变量定义*/
/*数组最大值，等待回复时间,不校验时的等待时间*/ 
#define MAX_temp_array 200
#define MAX_waittime 3000
#define normal_waittime 200


/*初始化错误时的尝试次数*/
#define esp8266_Init_Try_Time 5
/*********************************************************************************************/
/*底层函数*/

uint8_t* esp8266wifi_check_cmd(uint8_t *str_target,char *str_rx);
void esp8266wifi_reset(void);
void esp8266wifi_set_trans_mode(uint8_t mode);
void esp8266wifi_set_wifi_mode(uint8_t mode);
uint8_t esp8266wifi_send_cmd(char *cmd,uint8_t *expect,int waittime);
void esp8266wifi_close_connect(uint8_t ID);


/*5种模式发送数据,可单独调用*/
uint8_t mode_1_send_date(char *data);
uint8_t mode_2_send_date(char *Data);
uint8_t mode_2_send_date_IP(char *Data,char *SendIP,uint16_t SendPort);
void mode_3_and_4_send_date(char *data);
uint8_t mode_5_send_date(char *Data,uint8_t Link_ID);



/*********************************************************************************************/
/*用户调用函数*/
void esp8266_start_trans(void);
uint8_t esp8266_quit_trans(void);
uint8_t esp8266_Init(uint8_t RUNmode,uint8_t wifi_mode);
uint8_t esp8266_send_data(char *data,uint8_t ID);
uint8_t esp8266_solve_receive_data(char *true_data,uint8_t old_date[]);



/*********************************************************************************************/
/*初始化函数传参定义*/

/* 连接方式定义 */
#define mode_1_TCP_Client           1  /* 单连接TCP，esp做客户端 */
#define mode_2_UDP                  2  /* UDP传输 */
#define mode_3_TCP_transparent      3  /* TCP透传 */
#define mode_4_UDP_transparent      4  /* UDP透传 */
#define mode_5_TCP_Server           5  /* 多连接TCP，esp做服务器 */

/* wifi模式:1为客户端连接模式，2为热点模式，3为双开 */
#define wifi_mode_Station 1
#define wifi_mode_SoftAP 2
#define wifi_mode_SoftAP_Station 3

/*返回值状态*/
#define esp8266_OK 1
#define esp8266_ERROR 0



#endif
