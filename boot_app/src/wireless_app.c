/**
 * @file  wireless_app.c
 * @brief FreeRTOS integration for wireless_lib skeleton.
 *
 * Phase 0: schedule stub Wireless_Schedule() in a dedicated task.
 * RF/SBC/ADC/DAC remain TODO inside wireless_lib until platform ports land.
 */
#include "wireless_app.h"

#if BOOT_APP_WIRELESS_EN

#include "wireless_api.h"
#include "debug.h"
#include "cdc_debug.h"
#include "FreeRTOS.h"
#include "task.h"

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(xTimeInMs) \
	((TickType_t)(((TickType_t)(xTimeInMs) * (TickType_t)configTICK_RATE_HZ) / (TickType_t)1000))
#endif

static TaskHandle_t s_wl_task;

static void WirelessApp_OnConnected(uint8_t device_index)
{
	DBG("[WL] connected idx=%u\n", (unsigned)device_index);
	CDC_DBG_SYS("WL connected idx=%u\r\n", (unsigned)device_index);
}

static void WirelessApp_OnDisconnected(uint8_t device_index)
{
	DBG("[WL] disconnected idx=%u\n", (unsigned)device_index);
	CDC_DBG_SYS("WL disconnected idx=%u\r\n", (unsigned)device_index);
}

static void vWirelessTask(void *pvParameters)
{
	(void)pvParameters;

	DBG("[WL] schedule task running (role=%s)\n",
	    BOOT_APP_WIRELESS_ROLE_TX ? "TX/Slave" : "RX/Master");

	for (;;) {
		Wireless_Schedule();
		/* Yield frequently so USB/CDC Shell stay responsive. */
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

int App_WirelessStart(void)
{
	WirelessConfig_t cfg;
	int ret;

	cfg.role = BOOT_APP_WIRELESS_ROLE_TX
		? (uint8_t)WIRELESS_ROLE_SLAVE
		: (uint8_t)WIRELESS_ROLE_MASTER;
	cfg.sample_rate = SAMPLE_RATE;
	cfg.frame_size = ONE_FRAME;
	cfg.device_id = 0x00000002u; /* BanAirBundy product code */
	cfg.channel_num = (PACKET_AUDIO_CH > 0) ? (uint8_t)PACKET_AUDIO_CH : 1u;

	ret = Wireless_Init(&cfg);
	if (ret != 0) {
		DBG("[WL] Wireless_Init failed: %d\n", ret);
		return ret;
	}

	Wireless_RegisterConnectedCb(WirelessApp_OnConnected);
	Wireless_RegisterDisconnectedCb(WirelessApp_OnDisconnected);

	ret = Wireless_StartPairing();
	if (ret != 0) {
		DBG("[WL] StartPairing failed: %d\n", ret);
		return ret;
	}

	if (xTaskCreate(vWirelessTask,
			"WL",
			configMINIMAL_STACK_SIZE * 6,
			NULL,
			tskIDLE_PRIORITY + 2,
			&s_wl_task) != pdPASS) {
		DBG("[WL] create task failed\n");
		return -1;
	}

	DBG("[WL] started role=%u sr=%u frame=%u\n",
	    (unsigned)cfg.role,
	    (unsigned)cfg.sample_rate,
	    (unsigned)cfg.frame_size);
	CDC_DBG_SYS("WL started role=%u\r\n", (unsigned)cfg.role);
	return 0;
}

#endif /* BOOT_APP_WIRELESS_EN */
