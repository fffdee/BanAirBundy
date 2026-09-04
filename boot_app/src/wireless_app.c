/**
 * @file  wireless_app.c
 * @brief Bare-metal integration for wireless_lib (MVWIRE Turnkey 2_6).
 *
 * No RTOS. App_WirelessStart() brings the RF up directly during main() init;
 * the hard-real-time 2T1R state machine then lives in the BT_IRQn ISR at
 * priority 0 (highest), exactly like the vendor wireless_mic_tx/rx_sdk.
 * The super loop in main.c calls App_WirelessSchedule() -> Wireless_Schedule()
 * for the slow-path glue (audio pump, connection display). Because that glue
 * is cooperative and the RF timing is interrupt-driven, USB / Banux work in
 * the same loop can never starve the radio.
 */
#include "wireless_app.h"

#if BOOT_APP_WIRELESS_EN

#include "wireless_api.h"
#include "debug.h"

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

int App_WirelessStart(void)
{
	WirelessConfig_t cfg;
	int ret;

	WirelessApp_ConfigInit(&cfg);

	DBG("[WL] cfg en=%u role=%s mvwire=%u enc=%u dec=%u link=0x%02X%02X\n",
	    (unsigned)BOOT_APP_WIRELESS_EN,
	    BOOT_APP_WIRELESS_ROLE_TX ? "TX/Slave" : "RX/Master",
	    (unsigned)BOOT_APP_MVWIRE_EN,
	    (unsigned)ENCODE_CH,
	    (unsigned)DECODE_CH,
	    (unsigned)WIRELESS_LINK_KEY1,
	    (unsigned)WIRELESS_LINK_KEY0);

	DBG("[WL] bare-metal init (role=%s) - RF ISR on BT_IRQn prio 0\n",
	    BOOT_APP_WIRELESS_ROLE_TX ? "TX/Slave" : "RX/Master");

	/*
	 * Wireless_Init() runs the whole vendor bring-up (MvWire_StackInit ->
	 * WIRELESS_FUNCTION / DeviceRoleSet / 2T1R mode / Wireless_common_init)
	 * and enables BT_IRQn at priority 0. Allocations come from the T_Heap
	 * (T_HeapInit() must already have run in main()).
	 */
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

	DBG("[WL] started role=%u sr=%u frame=%u\n",
	    (unsigned)cfg.role,
	    (unsigned)cfg.sample_rate,
	    (unsigned)cfg.frame_size);
	return 0;
}

void App_WirelessSchedule(void)
{
	/* Slow-path RF glue; the real-time state machine is in the BT_IRQn ISR. */
	Wireless_Schedule();
}

void App_WirelessDiag(void)
{
#if BOOT_APP_MVWIRE_EN
	Wireless_DiagReport();
#endif
}

#endif /* BOOT_APP_WIRELESS_EN */
