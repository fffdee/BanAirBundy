/**
 * @file    rtos_api.h
 * @brief   FreeRTOS compatibility layer for Banux framework
 *          Maps proprietary rtos_api.h calls to standard FreeRTOS APIs
 */
#ifndef __RTOS_API_H__
#define __RTOS_API_H__

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"

/* Task API */
#define RTOS_TaskCreate(taskFunc, name, stackDepth, params, priority, handle) \
    xTaskCreate(taskFunc, name, stackDepth, params, priority, handle)

#define RTOS_TaskDelete(handle) vTaskDelete(handle)

#define RTOS_TaskDelay(ms) vTaskDelay(pdMS_TO_TICKS(ms))

#define RTOS_TaskDelayUntil(prevWakeTime, ms) \
    vTaskDelayUntil(prevWakeTime, pdMS_TO_TICKS(ms))

/* Semaphore API */
#define RTOS_SemaphoreCreateBinary() xSemaphoreCreateBinary()

#define RTOS_SemaphoreCreateMutex() xSemaphoreCreateMutex()

#define RTOS_SemaphoreTake(sem, timeout) xSemaphoreTake(sem, timeout)

#define RTOS_SemaphoreGive(sem) xSemaphoreGive(sem)

#define RTOS_SemaphoreDelete(sem) vSemaphoreDelete(sem)

/* Queue API */
#define RTOS_QueueCreate(length, itemSize) xQueueCreate(length, itemSize)

#define RTOS_QueueSend(queue, item, timeout) xQueueSend(queue, item, timeout)

#define RTOS_QueueReceive(queue, item, timeout) xQueueReceive(queue, item, timeout)

/* Event Group API */
#define RTOS_EventGroupCreate() xEventGroupCreate()

#define RTOS_EventGroupSetBits(eventGroup, bits) xEventGroupSetBits(eventGroup, bits)

#define RTOS_EventGroupWaitBits(eventGroup, bits, clearOnExit, waitForAll, timeout) \
    xEventGroupWaitBits(eventGroup, bits, clearOnExit, waitForAll, timeout)

/* Timer API */
#define RTOS_TimerCreate(name, periodMs, autoReload, callback, handle) \
    xTimerCreate(name, pdMS_TO_TICKS(periodMs), autoReload, NULL, callback)

#define RTOS_TimerStart(timer, timeout) xTimerStart(timer, timeout)

#define RTOS_TimerStop(timer, timeout) xTimerStop(timer, timeout)

/* Tick type */
#define RTOS_TickType TickType_t

#define RTOS_PORTMAX_DELAY portMAX_DELAY

/* Critical section */
#define RTOS_EnterCritical() taskENTER_CRITICAL()

#define RTOS_ExitCritical() taskEXIT_CRITICAL()

#endif /* __RTOS_API_H__ */
