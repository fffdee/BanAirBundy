/**
 * @file  cdc_upgrade.c
 * @brief USB CDC firmware upgrade bridge (bootloader port).
 */
#include "cdc_upgrade.h"
#include "app_upgrade.h"
#include "dual_partition.h"
#include "otg_device_cdc.h"
#include "reset.h"
#include "debug.h"
#include <string.h>

static int s_initialised  = 0;
static int s_upgrade_mode = 0;

static void cdc_tx(const uint8_t *buf, uint16_t len)
{
    OTG_DeviceCDC_Send((uint8_t *)buf, len);
}

static uint16_t cdc_rx_read_cb(uint8_t *buf, uint16_t maxLen)
{
    return OTG_DeviceCDC_Receive(buf, maxLen);
}

static int cdc_rx_avail_cb(void)
{
    return (int)OTG_DeviceCDC_GetRxCount();
}

static UpgradeChannel_t s_cdc_ch;

void CDC_Upgrade_Init(void)
{
    memset(&s_cdc_ch, 0, sizeof(s_cdc_ch));
    s_cdc_ch.id           = UPG_CH_CDC;
    s_cdc_ch.rx_read      = cdc_rx_read_cb;
    s_cdc_ch.tx_write     = cdc_tx;
    s_cdc_ch.rx_available = cdc_rx_avail_cb;
    s_upgrade_mode = 0;
    s_initialised  = 1;
    DBG("[CDC_UPG] init\n");
}

void CDC_Upgrade_EnterMode(void)
{
    uint8_t tmp[64];
    uint16_t n;
    const char *notice = "\r\n[UPG] Bootloader upgrade mode ready\r\n";

    if (!s_initialised) {
        return;
    }

    do {
        n = OTG_DeviceCDC_Receive(tmp, sizeof(tmp));
    } while (n > 0);

    OTG_DeviceCDC_Send((uint8_t *)notice, (uint16_t)strlen(notice));
    s_upgrade_mode = 1;
    DBG("[CDC_UPG] upgrade mode entered\n");
}

int CDC_Upgrade_InMode(void)
{
    return s_upgrade_mode;
}

int CDC_Upgrade_CheckEnter(void)
{
    uint8_t byte;

    if (!s_initialised || s_upgrade_mode) {
        return 0;
    }
    if (OTG_DeviceCDC_GetRxCount() == 0) {
        return 0;
    }

    if (OTG_DeviceCDC_PeekByte(&byte) == 1) {
        if (byte == UPG_SOF) {
            OTG_DeviceCDC_Receive(&byte, 1);
            s_upgrade_mode = 1;
            {
                uint8_t sof_pkt[1] = { UPG_SOF };
                App_Upgrade_InjectRaw(UPG_CH_CDC, sof_pkt, 1, cdc_tx);
            }
            return 1;
        }
    }
    return 0;
}

void CDC_Upgrade_Process(void)
{
    uint16_t n;

    if (!s_initialised || !s_upgrade_mode) {
        return;
    }

    n = OTG_DeviceCDC_GetRxCount();
    if (n > 0) {
        /* Do not DBG per RX burst during DATA — UART blocks USB and Windows
         * drops CDC (ClearCommError / 设备不识别此命令). */
        App_Upgrade_ProcessChannel(&s_cdc_ch);
    }

    if (App_Upgrade_IsFinished()) {
        DBG("[CDC_UPG] upgrade finished, rebooting...\n");
        {
            volatile uint32_t delay;
            for (delay = 0; delay < 100000; delay++) { ; }
        }
        Reset_McuSystem();
    }
}

int CDC_Upgrade_IsActive(void)
{
    return App_Upgrade_IsActive();
}
