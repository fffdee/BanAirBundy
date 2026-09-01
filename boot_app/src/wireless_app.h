/**
 * @file  wireless_app.h
 * @brief boot_app FreeRTOS glue for wireless_lib.
 */
#ifndef __WIRELESS_APP_H__
#define __WIRELESS_APP_H__

#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if BOOT_APP_WIRELESS_EN

/**
 * @brief Init wireless stack and start FreeRTOS wireless task.
 * @note  Call after heap init; safe after USB enum.
 * @return 0 on success, <0 on failure.
 */
int App_WirelessStart(void);

#else

static inline int App_WirelessStart(void) { return 0; }

#endif

#ifdef __cplusplus
}
#endif

#endif /* __WIRELESS_APP_H__ */
