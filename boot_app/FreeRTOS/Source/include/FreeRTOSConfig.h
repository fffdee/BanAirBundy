#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "app_config.h"

/* 使用 ROM 版本 FreeRTOS */
#define configUSE_PREEMPTION			1
#define configUSE_TICKLESS_IDLE			0
#define configCPU_CLOCK_HZ				(280000000UL)
#define configTICK_RATE_HZ				((TickType_t)1000)
#define configMAX_PRIORITIES			(5)
#define configMINIMAL_STACK_SIZE		((unsigned short)128)
#define configTOTAL_HEAP_SIZE			((size_t)(20 * 1024))
#define configMAX_TASK_NAME_LEN			(16)
#define configUSE_16_BIT_TICKS			0
#define configIDLE_SHOULD_YIELD			1
#define configUSE_TASK_NOTIFICATIONS		1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 1
#define configQUEUE_REGISTRY_SIZE		8
#define configUSE_QUEUE_SETS			0
#define configUSE_TIME_SLICING			1
#define configUSE_NEWLIB_REENTRANT		0
#define configENABLE_BACKWARD_COMPATIBILITY 0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0

/* Memory allocation */
#define configSUPPORT_STATIC_ALLOCATION		0
#define configSUPPORT_DYNAMIC_ALLOCATION		1

/* Hook functions */
#define configUSE_IDLE_HOOK				1
#define configUSE_TICK_HOOK				0
#define configCHECK_FOR_STACK_OVERFLOW	2
#define configUSE_MALLOC_FAILED_HOOK		1

/* Run time stats */
#define configGENERATE_RUN_TIME_STATS	0

/* Co-routine */
#define configUSE_CO_ROUTINES			0

/* Software timers */
#define configUSE_TIMERS				1
#define configTIMER_TASK_PRIORITY		(3)
#define configTIMER_QUEUE_LENGTH		10
#define configTIMER_TASK_STACK_DEPTH	(configMINIMAL_STACK_SIZE * 4)

/* API function inclusion */
#define INCLUDE_vTaskPrioritySet		1
#define INCLUDE_uxTaskPriorityGet		1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskCleanUpResources	0
#define INCLUDE_vTaskSuspend			1
#define INCLUDE_vTaskDelayUntil			1
#define INCLUDE_vTaskDelay				1
#define INCLUDE_xTaskGetSchedulerState	1
#define INCLUDE_xTimerPendFunctionCall	1
#define INCLUDE_uxTaskGetStackHighWaterMark	1

/* Interrupt priority */
#ifdef __NVIC_PRIO_BITS
 #define configPRIO_BITS __NVIC_PRIO_BITS
#else
 #define configPRIO_BITS 4
#endif
#define configKERNEL_INTERRUPT_PRIORITY		(0)
#define configMAX_SYSCALL_INTERRUPT_PRIORITY	(2)

/* FreeRTOS assert - avoid stdio.h since this header is included by .S files too */
#define configASSERT(x) if((x)==0) { for(;;); }

#endif /* FREERTOS_CONFIG_H */
