/**
 * @file  wireless_app.h
 * @brief boot_app bare-metal glue for wireless_lib (no RTOS).
 */
#ifndef __WIRELESS_APP_H__
#define __WIRELESS_APP_H__

#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if BOOT_APP_WIRELESS_EN

/**
 * @brief Init the wireless stack directly (bare-metal, no RTOS task).
 *        Brings up the RF (BT_IRQn) and starts pairing/advertising.
 * @note  Call after T_HeapInit(); BT_IRQn priority is set inside the stack.
 * @return 0 on success, <0 on failure.
 */
int App_WirelessStart(void);

/**
 * @brief Cooperative wireless poll - call from the bare-metal super loop.
 *        Drives Wireless_Schedule() (RF slow-path glue + audio pump).
 *        The hard-real-time RF state machine runs in the BT_IRQn ISR (prio 0),
 *        so this only needs to be called often, not on a strict cadence.
 */
void App_WirelessSchedule(void);

/**
 * @brief Print a read-only RF liveness report (conn/sync/TX scan state).
 *        Call periodically (~2s) from the super loop to see whether the
 *        radio state machine is advancing. No effect on the RF.
 */
void App_WirelessDiag(void);

#else

static inline int  App_WirelessStart(void) { return 0; }
static inline void App_WirelessSchedule(void) {}
static inline void App_WirelessDiag(void) {}

#endif

#ifdef __cplusplus
}
#endif

#endif /* __WIRELESS_APP_H__ */
