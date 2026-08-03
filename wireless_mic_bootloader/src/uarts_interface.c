/* Wrapper: compile the unified SDK UARTs interface source as part of bootloader/src */
#include "../../wireless_mic_unified_sdk/banux/02_device_drivers/driver/driver_api/src/uarts_interface.c"

/* Legacy single-port send wrappers used by retarget.c */
void UART0_SendByte(uint8_t SendByte)
{
    UARTS_SendByte(UART_PORT0, SendByte);
}

void UART1_SendByte(uint8_t SendByte)
{
    UARTS_SendByte(UART_PORT1, SendByte);
}
