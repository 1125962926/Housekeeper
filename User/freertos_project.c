#include "string.h"
#include "freertos_project.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/KEY/key.h"
#include "./BSP/ESP8266/8266wifi.h"
#include "./BSP/RC522/RC522.h"
#include "./BSP/OLED/oled.h"
#include "./BSP/TIMER/gtim.h"
/*FreeRTOS*********************************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
/******************************************************************************************************/
/*FreeRTOS配置*/

/* START_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */       
#define START_TASK_PRIO 1								/* 任务优先级 */
#define START_TASK_STACK_SIZE 128						/* 任务堆栈大小 */
TaskHandle_t start_task_handler;						/* 任务句柄，动态分配需要传参，静态分配作为返回值 */
void start_task(void *pvParameters);					/* 任务函数 */

/* TASK1 任务 处理esp8266接收到的数据
 */
#define TASK1_PRIO 3
#define TASK1_STACK_SIZE 512
TaskHandle_t task1_handler;
void task1(void * pvParameters);

/* TASK2 任务 处理RFID刷卡认证
 */
#define TASK2_PRIO 3
#define TASK2_STACK_SIZE 128
TaskHandle_t task2_handler;
void task2(void * pvParameters);

/* TASK3 任务 空闲时OLED屏保
 */
#define TASK3_PRIO 2
#define TASK3_STACK_SIZE 128
TaskHandle_t task3_handler;
void task3(void * pvParameters);

/* TASK4 任务 舵机
 */
#define TASK4_PRIO 4
#define TASK4_STACK_SIZE 128
TaskHandle_t task4_handler;
void task4(void * pvParameters);

/******************************************************************************************************/
/* 事件标志组 */
EventGroupHandle_t EventGroup_Task;

#define EVENTBIT_0	(1<<0)				//事件位
#define EVENTBIT_1	(1<<1)
#define EVENTBIT_2	(1<<2)
#define EVENTBIT_ALL	(EVENTBIT_0|EVENTBIT_1|EVENTBIT_2)
/******************************************************************************************************/
/* 软件定时器 */
TimerHandle_t Timer1_Task;
// TimerHandle_t Timer2_Task;

void Timer1_Task_Callback(TimerHandle_t xTimer);
void Timer2_Task_Callback(TimerHandle_t xTimer);

/******************************************************************************************************/
/* 低功耗函数 */
void PRE_SLEEP_PROCESSING(uint32_t ulExpectedIdleTime)
{
	__HAL_RCC_GPIOA_CLK_DISABLE();
	__HAL_RCC_GPIOB_CLK_DISABLE();
	__HAL_RCC_GPIOC_CLK_DISABLE();
	__HAL_RCC_GPIOD_CLK_DISABLE();
	__HAL_RCC_GPIOE_CLK_DISABLE();
	__HAL_RCC_GPIOF_CLK_DISABLE();
	__HAL_RCC_GPIOG_CLK_DISABLE();
	__HAL_RCC_GPIOI_CLK_DISABLE();
}

void POST_SLEEP_PROCESSING(uint32_t ulExpectedIdleTime)
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_GPIOI_CLK_ENABLE();
}
/******************************************************************************************************/
/* 钩子函数 */

/* 空闲任务 */
void vApplicationIdleHook(void)
{
	
}


/******************************************************************************************************/
/**
 * @brief       FreeRTOS例程入口函数
 * @param       无
 * @retval      无
 */
void freertos_init(void)
{    
	/* 创建事件标志组 */
	EventGroup_Task = xEventGroupCreate();
	xEventGroupClearBits(EventGroup_Task, 0x00FF);

	/* 创建定时器 */
	/* 单次 */
	Timer1_Task = xTimerCreate("Timer1_Task", 5000, pdFALSE, 
		(void *)1, Timer1_Task_Callback);

	/* 创建任务 */
	xTaskCreate(
		(TaskFunction_t) 			start_task,      			/* 指向任务函数的指针 */
		(char *) 					"start_task",      			/* 任务名字，最大长度 configMAX_TASK_NAME_LEN */
		(configSTACK_DEPTH_TYPE) 	START_TASK_STACK_SIZE,     	/* 任务堆栈大小，单位为字 */
		(void *) 					NULL,      					/* 传递给任务函数的参数 */
		(UBaseType_t) 				START_TASK_PRIO,         	/* 任务优先级，范围：0 ~ configMAX_PRIORITIES - 1 */
		(TaskHandle_t *) 			&start_task_handler 		/* 任务句柄，任务的任务控制块TCB */
	);
    vTaskStartScheduler();
}

/**
 * @brief       start_task
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           /* 进入临界区 */
	
    /* 创建任务1，esp8266 */
	xTaskCreate(
		(TaskFunction_t) 			task1,
		(char *) 					"task1_ESP8266",
		(configSTACK_DEPTH_TYPE) 	TASK1_STACK_SIZE,
		(void *) 					NULL,
		(UBaseType_t) 				TASK1_PRIO,
		(TaskHandle_t *) 			&task1_handler
	);

    /* 创建任务2，RFID */
	xTaskCreate(
		(TaskFunction_t) 			task2,
		(char *) 					"task2_RFID",
		(configSTACK_DEPTH_TYPE) 	TASK2_STACK_SIZE,
		(void *) 					NULL,
		(UBaseType_t) 				TASK2_PRIO,
		(TaskHandle_t *) 			&task2_handler
	);

	/* 创建任务3，空闲时OLED屏保 */
	xTaskCreate(
		(TaskFunction_t) 			task3,
		(char *) 					"task3_OLED",
		(configSTACK_DEPTH_TYPE) 	TASK3_STACK_SIZE,
		(void *) 					NULL,
		(UBaseType_t) 				TASK3_PRIO,
		(TaskHandle_t *) 			&task3_handler
	);

	/* 创建任务4，舵机 */
	xTaskCreate(
		(TaskFunction_t) 			task4,
		(char *) 					"task4_Servos",
		(configSTACK_DEPTH_TYPE) 	TASK4_STACK_SIZE,
		(void *) 					NULL,
		(UBaseType_t) 				TASK4_PRIO,
		(TaskHandle_t *) 			&task4_handler
	);
    vTaskDelete(NULL); /* 删除开始任务 */
    taskEXIT_CRITICAL();            /* 退出临界区 */
}

/**
 * @brief       task1：处理esp8266接收到的数据
 * @param       pvParameters : 传入参数
 * @retval      无
 */
void task1(void * pvParameters)
{
	char *rx_data = NULL;/* 接收缓冲 */
	while(1)
	{
		if(esp8266wifi_rx_sta & 0X8000)//串口2esp收到的信息通过串口1打印
        {
			rx_data = pvPortCalloc(1, 32);/* 接收缓冲 */
	
			taskENTER_CRITICAL();
			{
				esp8266_solve_receive_data(rx_data, esp8266wifi_rx_buf);
				if(!strcmp((const char*)rx_data, "abc123"))
				{
					xEventGroupSetBits(EventGroup_Task, EVENTBIT_0);
				}
				esp8266wifi_rx_sta = 0;
			}
            taskEXIT_CRITICAL();

			vPortFree(rx_data);
        }
		vTaskDelay(100);
	}
}

/**
 * @brief       task2：处理RFID刷卡认证
 * @param       pvParameters : 传入参数
 * @retval      无
 */
void task2(void * pvParameters)
{
	int8_t ID;
	while(1)
	{
		if(Wait_RFID_card() == MI_OK)
		{
			// vTaskSuspendAll();
			taskENTER_CRITICAL();
			{
				ID = GET_card_ID();
				if(ID == 0)
					//printf("验证失败, 无效卡\n");
					OLED_ShowString(1, 2, "ERROR card!", 16);
				if(ID > 0)
				{
					//printf("用户 %d, 欢迎回家!\n", ID);
					xEventGroupSetBits(EventGroup_Task, EVENTBIT_1);
				}
			}
            taskEXIT_CRITICAL();			
			// (void)xTaskResumeAll();
		}
		vTaskDelay(100);
	}
}

/**
 * @brief       task3：空闲时OLED屏保，如果发生开门事件，则延时一段时间再刷新OLED
 * @param       pvParameters : 传入参数
 * @retval      无
 */
void task3(void * pvParameters)
{
	uint8_t i = 0;
	TickType_t pxPreviousWakeTime;
	while(1)
	{		
		/* 如果接收到任务通知，把通知值清 0，不阻塞等待 */
		if(ulTaskNotifyTake(pdTRUE, 0))
		{	
			/* 设置软件定时器，并挂起自身，超时再唤醒 */
			xTimerStart(Timer1_Task, portMAX_DELAY);
			vTaskSuspend(NULL);/* 挂起自身 */
		}

		/* 记录当前时间 */
		pxPreviousWakeTime = xTaskGetTickCount();

		/* 屏保显示 */
		OLED_DrawBMP(0, 0, i++, frame_len, frame_width);
		OLED_Refresh_Gram();
		if(i >= page_sum)
			i = 0;

		/* 采用绝对延时，保证刷新时间可控 */
		vTaskDelayUntil(&pxPreviousWakeTime, refresh_speed);
	}
}

/**
 * @brief       task4：舵机开关门
 * @param       pvParameters : 传入参数
 * @retval      无
 */
void task4(void * pvParameters)
{
	volatile EventBits_t EventValue;
	while(1)
	{		
		/* 等待其中一个事件完成，皆可启动舵机 */
		EventValue = xEventGroupWaitBits(EventGroup_Task, EVENTBIT_ALL, pdTRUE, pdFALSE, portMAX_DELAY);

		/* 保留欢迎语一段时间，再进行OLED屏保 */
		xTaskNotifyGive(task3_handler);

		OLED_Clear();

		switch (EventValue)
		{
			case EVENTBIT_0:
				//printf("WIFI开门成功\r\n");
				OLED_ShowString(1, 2, "WIFI open door!", 16);
				break;
			case EVENTBIT_1:
				//printf("刷卡开门成功\r\n");
				OLED_ShowString(1, 2, "RFID open door!", 16);
				break;
			case EVENTBIT_2:
				//printf("密码开门成功\r\n");
				OLED_ShowString(1, 2, "PassWord open door!", 16);
				break;
		}
		OLED_Refresh_Gram();

		/* 舵机偏转开门 */
		set_Angle(180);

		/* 保持开锁 5秒 */
		vTaskDelay(5000);

		/* 舵机偏转锁门 */
		set_Angle(0);

		vTaskDelay(10);
	}
}



/* 软件定时器回调函数 */
void Timer1_Task_Callback(TimerHandle_t xTimer)
{
	/* 唤醒屏保显示 */
	vTaskResume(task3_handler);
}


void Timer2_Task_Callback(TimerHandle_t xTimer)
{
	
}

