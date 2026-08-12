/**
 * @file Banux.h
 * @brief Banux boot framework single-header public API.
 *
 * Include this file from an application instead of including each Banux
 * subsystem separately.  The implementation remains in the existing
 * framework modules; this file is the stable application-facing facade.
 */
#ifndef BANUX_H
#define BANUX_H

#include "banux_config.h"
#include "01_hal_drivers/hal_drivers.h"
#include "03_driver_framework/drv_init.h"
#include "03_driver_framework/event/bg_event.h"
#include "05_component/sys_state/sys_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*BanuxCallback_t)(void);

/** Initialise the framework, drivers, event bus and system state. */
int Banux_begin(void);

/** Execute one cooperative framework iteration and the application callback. */
void Banux_loop(void);

/** Run forever, Arduino-style. Returns only if the application callback does. */
void Banux_run(void);

/** Install application callbacks (either callback may be NULL). */
void Banux_setSetup(BanuxCallback_t callback);
void Banux_setLoop(BanuxCallback_t callback);

/** C-friendly Arduino compatibility hooks. Define these in the application. */
void setup(void);
void loop(void);
void Banux_setup(void);
void Banux_loopCallback(void);

/* Optional convenience aliases for Arduino-like sketches. */
#define BANUX_SETUP()       Banux_setup()
#define BANUX_LOOP()        Banux_loopCallback()

#ifdef __cplusplus
}
#endif

#endif /* BANUX_H */
