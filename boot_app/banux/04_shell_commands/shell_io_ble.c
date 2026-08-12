/*
 * boot_app exposes the Shell through USB CDC only.  These no-op BLE hooks
 * retain compatibility with the shared bg_shell implementation.
 */
#include "shell_io_ble.h"

uint8_t g_is_sync_command;

void BLE_BufferSyncResponse(const char *data, uint16_t len)
{
    (void)data;
    (void)len;
}
