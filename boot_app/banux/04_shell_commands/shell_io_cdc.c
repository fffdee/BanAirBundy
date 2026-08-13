/**
 * @file shell_io_cdc.c
 * @brief USB CDC ACM transport implementation for BanUX Shell.
 */
#include "shell_io_cdc.h"
#include "otg_device_cdc.h"

static uint16_t ShellIO_CDC_Send(uint8_t *data, uint16_t len)
{
    /* DTR=0 means host has not opened COM — Bulk-IN would time out. */
    if (!data || !len || !UsbCDC.InitOk || !UsbCDC.ControlLineState.DTR)
        return 0;

    return OTG_DeviceCDC_Send(data, len);
}

static uint16_t ShellIO_CDC_Recv(uint8_t *data, uint16_t max_len)
{
    if (!data || !max_len || !UsbCDC.InitOk)
        return 0;

    return OTG_DeviceCDC_Receive(data, max_len);
}

static uint16_t ShellIO_CDC_Available(void)
{
    return UsbCDC.InitOk ? OTG_DeviceCDC_GetRxCount() : 0;
}

static const ShellIO_t g_shell_io_cdc = {
    "CDC",
    ShellIO_CDC_Send,
    ShellIO_CDC_Recv,
    ShellIO_CDC_Available,
};

const ShellIO_t *ShellIO_CDC_Get(void)
{
    return &g_shell_io_cdc;
}
