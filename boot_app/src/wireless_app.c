/**
 * @file  wireless_app.c
 * @brief FreeRTOS integration for wireless_lib (MVWIRE Turnkey 2_6).
 */
#include "wireless_app.h"

#if BOOT_APP_WIRELESS_EN

#include "wireless_api.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(xTimeInMs) \
	((TickType_t)(((TickType_t)(xTimeInMs) * (TickType_t)configTICK_RATE_HZ) / (TickType_t)1000))
#endif

/* Static stack: avoid heap adjacency smash into TCB when Assoc/SBC deep-calls. */
#ifndef WL_TASK_STACK_WORDS
#define WL_TASK_STACK_WORDS  4096u
#endif

static TaskHandle_t s_wl_task;
static StackType_t s_wl_stack[WL_TASK_STACK_WORDS];

static void WirelessApp_ConfigInit(WirelessConfig_t *cfg)
{
	cfg->role = BOOT_APP_WIRELESS_ROLE_TX
		? (uint8_t)WIRELESS_ROLE_SLAVE
		: (uint8_t)WIRELESS_ROLE_MASTER;
	cfg->sample_rate = SAMPLE_RATE;
	cfg->frame_size = ONE_FRAME;
	cfg->device_id = 0x00000002u; /* BanAirBundy product code */
	cfg->channel_num = (PACKET_AUDIO_CH > 0) ? (uint8_t)PACKET_AUDIO_CH : 1u;
}

static void WirelessApp_OnConnected(uint8_t device_index)
{
	static uint8_t log_count;

	if (log_count < 12u) {
		DBG("[WL] connected idx=%u\n", (unsigned)device_index);
		log_count++;
	}
}

static void WirelessApp_OnDisconnected(uint8_t device_index)
{
	static uint8_t log_count;

	if (log_count < 12u) {
		DBG("[WL] disconnected idx=%u\n", (unsigned)device_index);
		log_count++;
	}
}

static void vWirelessTask(void *pvParameters)
{
	WirelessConfig_t cfg;
	int ret;
	unsigned n = 0;

	(void)pvParameters;

	DBG("[WL] schedule task running (role=%s) stk=%u words (static)\n",
	    BOOT_APP_WIRELESS_ROLE_TX ? "TX/Slave" : "RX/Master",
	    (unsigned)WL_TASK_STACK_WORDS);

	WirelessApp_ConfigInit(&cfg);
	ret = Wireless_Init(&cfg);
	if (ret != 0) {
		DBG("[WL] Wireless_Init failed: %d\n", ret);
		for (;;)
			vTaskDelay(pdMS_TO_TICKS(1000));
	}

	Wireless_RegisterConnectedCb(WirelessApp_OnConnected);
	Wireless_RegisterDisconnectedCb(WirelessApp_OnDisconnected);

	ret = Wireless_StartPairing();
	if (ret != 0) {
		DBG("[WL] StartPairing failed: %d\n", ret);
		for (;;)
			vTaskDelay(pdMS_TO_TICKS(1000));
	}

	DBG("[WL] started role=%u sr=%u frame=%u stk=%u\n",
	    (unsigned)cfg.role,
	    (unsigned)cfg.sample_rate,
	    (unsigned)cfg.frame_size,
	    (unsigned)WL_TASK_STACK_WORDS);
	for (;;) {
		if (n < 3u)
			DBG("[WL] sched #%u enter\n", n);

		Wireless_Schedule();

		if (n < 3u) {
			DBG("[WL] sched #%u ok hwm=%u\n",
			    n,
			    (unsigned)uxTaskGetStackHighWaterMark(NULL));
		} else if ((n % 1000u) == 0u) {
			DBG("[WL] alive n=%u hwm=%u\n",
			    n,
			    (unsigned)uxTaskGetStackHighWaterMark(NULL));
		}
		n++;

		/* Yield frequently so USB/CDC Shell stay responsive. */
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

int App_WirelessStart(void)
{
	WirelessConfig_t cfg;

	WirelessApp_ConfigInit(&cfg);

	DBG("[WL] cfg en=%u role=%s mvwire=%u enc=%u dec=%u link=0x%02X%02X\n",
	    (unsigned)BOOT_APP_WIRELESS_EN,
	    BOOT_APP_WIRELESS_ROLE_TX ? "TX/Slave" : "RX/Master",
	    (unsigned)BOOT_APP_MVWIRE_EN,
	    (unsigned)ENCODE_CH,
	    (unsigned)DECODE_CH,
	    (unsigned)WIRELESS_LINK_KEY1,
	    (unsigned)WIRELESS_LINK_KEY0);

	/*
	 * Use xTaskGenericCreate with a BSS stack buffer so overflow of the
	 * deep AudioAssociation/SBC path does not immediately smash a heap TCB.
	 */
	if (xTaskGenericCreate(vWirelessTask,
			       "WL",
			       (uint16_t)WL_TASK_STACK_WORDS,
			       NULL,
			       tskIDLE_PRIORITY + 2,
			       &s_wl_task,
			       s_wl_stack,
			       NULL) != pdPASS) {
		DBG("[WL] create task failed\n");
		return -1;
	}
	return 0;
}

#endif /* BOOT_APP_WIRELESS_EN */
