/*
 * BLE transport is not included in boot_app.  bg_shell.c keeps this header
 * for shared BanUX source compatibility.
 */
#ifndef __SHELL_IO_BLE_H__
#define __SHELL_IO_BLE_H__

#include "type.h"

extern uint8_t g_is_sync_command;

void BLE_BufferSyncResponse(const char *data, uint16_t len);

#endif /* __SHELL_IO_BLE_H__ */
