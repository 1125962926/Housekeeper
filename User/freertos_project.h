#ifndef __FREERTOS_DEMO_H
#define __FREERTOS_DEMO_H

// #include "FreeRTOS.h"
// #include "task.h"

void freertos_init(void);

/* 低功耗函数 */
void PRE_SLEEP_PROCESSING(unsigned int ulExpectedIdleTime);
void POST_SLEEP_PROCESSING(unsigned int ulExpectedIdleTime);

/* 钩子函数 */
void vApplicationIdleHook( void );

#endif
