/**
 * @file mvwire_stack.c
 * @brief Slim WirelessInit / ConnectedCB for Turnkey 2_6
 */
#include "mvwire_port.h"

#if BOOT_APP_MVWIRE_EN

#include "app_config.h"
#include "sbc_encoder.h"
#include "bb_api.h"
#include "wireless2.h"
#include "wireless_usr_api.h"
#include "audio_association.h"
#include "chip_info.h"
#include "debug.h"
#include "irqn.h"
#include "type.h"
#include <string.h>

#ifndef RFAUDIO_TRANS_LEN
#define RFAUDIO_TRANS_LEN  RFAUDIO_TRANS_A
#endif

const uint32_t CompanyWord =
	((uint32_t)COMPANY_BYTE3 << 24) |
	((uint32_t)COMPANY_BYTE2 << 16) |
	((uint32_t)WIRELESS_LINK_KEY1 << 8) |
	((uint32_t)WIRELESS_LINK_KEY0);

uint32_t WirelessDeviceId =
	((~COMPANY_BYTE3) & 0xff) |
	(((~COMPANY_BYTE2) & 0xff) << 8) |
	(((~WIRELESS_LINK_KEY1) & 0xff) << 16) |
	(((~WIRELESS_LINK_KEY0) & 0xff) << 24);

unsigned char audio_init_isready;
uint8_t syncdevice2_thold;
uint32_t em_start_addr = BB_EM_START_PARAMS;

/* Pairing callbacks (no flash lock — open pairing like default 1532). */
static unsigned char PairFlag_None(void) { return 0; }
static uint32_t PairInfo_None(unsigned char Device)
{
	(void)Device;
	return NOPAIR_WORD;
}
static bool PairInfoSet_None(uint32_t Info, unsigned char Device)
{
	(void)Info;
	(void)Device;
	return TRUE;
}

PairedFlagGetCallback PairedFlagGetFunc;
PairInfoGetCallback PairInfoGetFunc;
PairedInfoSetCallback PairedInfoSetFunc;
PairChipTpyeGetCallback PairChipTpyeGetFunc;
PairedChipTpyeSetCallback PairedChipTpyeSetFunc;

Audio_Check1stFrameAllRight_fp Audio_Check1stFrameAllRight_cb;
Audio_Check1stFrameAllRightStateGet_fp Audio_Check1stFrameAllRightStateGet_cb;
Audio_Check1stFrameAllRightCounterStart_fp Audio_Check1stFrameAllRightCounterStart_cb;
wireless_AudioParityCntStart_fp wireless_AudioParityCntStart_cb;
wireless_AudioParityCntProc_fp wireless_AudioParityCntProc_cb;

static uint8_t s_rfsend_buffer[RFAUDIO_TRANS_A + 24];
static Wireless2_param_t s_wireless2_config;
static void (*s_app_conn_cb)(uint8_t);
static void (*s_app_disc_cb)(uint8_t);
static uint8_t s_role_tx;

/* BT_IRQn entry counter, defined in main.c (BtIrqProbe bumps it on every RF
 * interrupt). Printed by Wireless_DiagReport() as the decisive RF-liveness
 * signal: climbing == radio ISR alive; stuck at 0 == baseband never started. */
extern volatile uint32_t g_bt_irq_count;

/*
 * Part of the official WirelessStackTask() main loop (slave RSSI / power
 * control). It is in libwireless2.a but not exported by wireless2.h.
 */
extern void MVWIRE2_slave_schedule(void);

/* 1st-frame sync helpers (RX 2_6) */
static unsigned char s_dev0_sync = 0;
static unsigned char s_dev1_sync = 0;
static unsigned char s_dev0_hiscnt;
static unsigned char s_dev1_hiscnt;
unsigned char Device0_2rdPackSample;
unsigned char Device1_2rdPackSample;

void Audio_Check1stFrameAllRightCounterReset(unsigned char id)
{
	if (id == 0)
		s_dev0_sync = 0;
	else
		s_dev1_sync = 0;
}

void Audio_Check1stFrameAllRightCounterResetForAudioAssociation(uint8_t id)
{
	Audio_Check1stFrameAllRightCounterReset(id);
}

void Audio_Check1stFrameAllRightCounterStart(unsigned char id)
{
	if (id == 0)
		s_dev0_sync = 1;
	else
		s_dev1_sync = 1;
}

unsigned char Audio_Check1stFrameAllRightStateGet(unsigned char id)
{
	return (id == 0) ? s_dev0_sync : s_dev1_sync;
}

unsigned char Audio_Check1stFrameAllRight(unsigned char id, unsigned char cnt)
{
	static unsigned char c0, c1;

	if ((id == 0) && (s_dev0_sync == 1)) {
		if ((s_dev0_hiscnt == 0) || (s_dev0_hiscnt != cnt))
			c0 = 0;
		if ((cnt != 0) && (s_dev0_hiscnt == cnt)) {
			c0++;
			if (c0 >= 2) {
				s_dev0_sync = 2;
			}
		}
		s_dev0_hiscnt = cnt;
		return 0;
	}
	if ((id == 1) && (s_dev1_sync == 1)) {
		if ((s_dev1_hiscnt == 0) || (s_dev1_hiscnt != cnt))
			c1 = 0;
		if ((cnt != 0) && (s_dev1_hiscnt == cnt)) {
			c1++;
			if (c1 >= 2)
				s_dev1_sync = 2;
		}
		s_dev1_hiscnt = cnt;
		return 0;
	}
	if ((id == 0) && (s_dev0_sync == 2))
		return 1;
	if ((id == 1) && (s_dev1_sync == 2))
		return 1;
	return 0;
}

static void ConfigWirelessBbParams(WirelessBbParams *params)
{
	memset(params, 0, sizeof(*params));
	params->em_start_addr = BB_EM_START_PARAMS;
	params->pAgcDisable = 0;
	params->pAgcLevel = 1;
	params->pSniffNego = 0;
	params->pSniffInterval = 0x320;
	params->pSniffAttempt = 0x01;
	params->pSniffTimeout = 0x01;
}

static void WirelessDeviceIdInit(void)
{
	uint64_t ChipID = 0;

	Chip_IDGet(&ChipID);
	ChipID = (ChipID >> 32) & 0xffffffffull;
	if (ChipID != 0 && ChipID != 0xffffffffull)
		WirelessDeviceId = (uint32_t)ChipID;
}

static void PairingInit(void)
{
	PairedFlagGetFunc = PairFlag_None;
	PairInfoGetFunc = PairInfo_None;
	PairedInfoSetFunc = PairInfoSet_None;
}

void MvWire_AudioReadySet(uint8_t ready)
{
	audio_init_isready = ready ? 1 : 0;
}

void MVWIRE2_ConnectedCB(unsigned char id, unsigned char role)
{
	(void)role;
	if (!audio_init_isready)
		return;

	if ((id == 0) && (device1.ConStatus != CONNECT_AUDIO)) {
		device1.ConStatus = CONNECT_AUDIO;
		device1.handle = 0x80;
		device1.RecvNum = s_role_tx ? 0 : 30;
		if (s_role_tx)
			MvWire_TransBufInit();
		if (s_app_conn_cb)
			s_app_conn_cb(0);
	} else if ((id == 1) && (device2.ConStatus != CONNECT_AUDIO)) {
		device2.ConStatus = CONNECT_AUDIO;
		device2.handle = 0x81;
		device2.RecvNum = 30;
		if (s_app_conn_cb)
			s_app_conn_cb(1);
	}
}

void MVWIRE2_DisconnectedCB(unsigned char id, unsigned char role)
{
	(void)role;
	if ((id == 0) && (device1.ConStatus != CONNECT_NONE)) {
		device1.handle = HANDLE_NONE;
		device1.ConStatus = CONNECT_NONE;
		Audio_Check1stFrameAllRightCounterReset(0);
		WirelessAudioDevice1RxSyncReset();
		if (s_role_tx)
			MvWire_TransBufInit();
		if (s_app_disc_cb)
			s_app_disc_cb(0);
	} else if ((id == 1) && (device2.ConStatus != CONNECT_NONE)) {
		device2.handle = HANDLE_NONE;
		device2.ConStatus = CONNECT_NONE;
		Audio_Check1stFrameAllRightCounterReset(1);
		WirelessAudioDevice2RxSyncReset();
		if (s_app_disc_cb)
			s_app_disc_cb(1);
	}
}

char MVWIRE2_GetDevice1ConnState(void)
{
	return device1.ConStatus != CONNECT_NONE;
}

char MVWIRE2_GetDevice2ConnState(void)
{
	return device2.ConStatus != CONNECT_NONE;
}

void MVWIRE2_ConnStateDisplay(void)
{
	static char f1, f2;
	char c1 = MVWIRE2_GetDevice1ConnState();
	char c2 = MVWIRE2_GetDevice2ConnState();

	if (f1 != c1) {
		DBG(c1 ? "device1_conn\r\n" : "device1_disconn\r\n");
		f1 = c1;
	}
	if (f2 != c2) {
		DBG(c2 ? "device2_conn\r\n" : "device2_disconn\r\n");
		f2 = c2;
	}
}

void MvWire_StackSchedule(void)
{
	/*
	 * Vendor SDK WirelessStackTask() calls MVWIRE2_slave_schedule() ONLY
	 * under #if defined(WIRELESS_TURNKEY1_4). For Turnkey 2_6 (this build)
	 * the reference TX/RX binaries call ONLY MVWIRE2_ConnStateDisplay();
	 * the objdump of wireless_mic_tx_sdk WirelessStackTask confirms the
	 * slave_schedule branch is compiled out. Calling it here (as the old
	 * runtime `if (s_role_tx)` did) disturbed the TX radio state machine.
	 */
#if defined(WIRELESS_TURNKEY1_4)
	if (s_role_tx)
		MVWIRE2_slave_schedule();
#endif
	MVWIRE2_ConnStateDisplay();
}

uint8_t MvWire_GetDeviceStatus(uint8_t device_index)
{
	if (device_index == 0)
		return (uint8_t)device1.ConStatus;
	if (device_index == 1)
		return (uint8_t)device2.ConStatus;
	return CONNECT_NONE;
}

uint8_t MvWire_1stFrameSyncState(uint8_t id)
{
	return (uint8_t)Audio_Check1stFrameAllRightStateGet((unsigned char)id);
}

/*
 * Read-only RF liveness report, called from the super loop about every 2s.
 *
 * common_init() (main CPU) always builds the sync table, so the boot log looks
 * healthy even when the radio never links. The DECISIVE field is bt_irq: the
 * BT_IRQn ISR entry counter (crt0.S repoints ISR_TABLE[15] to BtIrqProbe() in
 * main.c, which counts then tail-calls the vendor rwbt_isr()).
 *   - bt_irq climbing  => the radio baseband IS generating interrupts and the
 *     2T1R state machine runs. A still-frozen d1/d2/sync then means "RF alive
 *     but no link with the peer" -> both boards must be powered at once; if
 *     they are, it is a pairing/frequency/config problem, not an ISR problem.
 *   - bt_irq stuck at 0 => the RF ISR never fires -> the baseband was never
 *     started (power/clock/init), a deeper fault than a missing peer.
 * conn_mode must be 1 (auto); dev must be 1 (RX master) / 2 (TX slaver).
 *
 * NOTE: MVWIRE2_DBG_FUNC() was removed -- its swbb_freq_band_dbg/hosc_cap_dbg
 * dumps are gated behind vendor dbg-enable flags that are off by default, so
 * it prints nothing. wireless_slave_1tnr_get_conn_state() was also removed --
 * it is a 1T1R API returning a meaningless constant in this 2T1R build.
 */
void Wireless_DiagReport(void)
{
	DBG("[RF] role=%s dev=%u conn_mode=%u bt_irq=%lu d1=%u d2=%u sync0=%u sync1=%u\n",
	    s_role_tx ? "TX/slave" : "RX/master",
	    (unsigned)MVWIRE2_DeviceGet(),
	    (unsigned)MVWIRE2_2T1R_Get_Conn_Mode(),
	    (unsigned long)g_bt_irq_count,
	    (unsigned)device1.ConStatus, (unsigned)device2.ConStatus,
	    (unsigned)Audio_Check1stFrameAllRightStateGet(0),
	    (unsigned)Audio_Check1stFrameAllRightStateGet(1));
}

/* Allow wireless_tx/rx to register app-level callbacks into stack CB. */
void MvWire_RegisterAppConnCb(void (*conn)(uint8_t), void (*disc)(uint8_t))
{
	s_app_conn_cb = conn;
	s_app_disc_cb = disc;
}

int MvWire_StackInit(uint8_t role_tx)
{
	WirelessBbParams params;
	uint32_t em_need;

	s_role_tx = role_tx ? 1 : 0;
	audio_init_isready = 0;
	PairingInit();
	WirelessDeviceIdInit();

	WIRELESS_FUNCTION(BB_MPU_START_ADDR);
	MVWIRE2_DeviceRoleSet(role_tx ? MVWIRE2_SLAVER_ROLE : MVWIRE2_MASTER_ROLE);
#ifdef MV_WIRELESS2_MODE2
	MVWIRE2_2T1R_Set_TxMode(MV_WIRELESS2_PARAM1);
	MVWIRE2_SetWorkMode(MV_WIRELESS2_PARAM2);
#endif

	/* WirelessInit (2_6) */
	if (get_OOB_band() == 1)
		wireless2_set_afh(0);
	else
		wireless2_set_afh(1);

	em_need = wireless_em_size();
	if (em_need % 4096)
		em_need = ((em_need / 4096) + 1) * 4096;
	if (em_need > BB_EM_SIZE) {
		DBG("[WL] BB_EM_SIZE too small\n");
		return -1;
	}

	ConfigWirelessBbParams(&params);
	memset(&s_wireless2_config, 0, sizeof(s_wireless2_config));
	s_wireless2_config.npack = RFPACK_NAUDIO;
	s_wireless2_config.rf_pbuffer = s_rfsend_buffer;
	s_wireless2_config.rf_translen = RFAUDIO_TRANS_LEN;
#if defined(ENCODE_CH) && (ENCODE_CH != 0)
	s_wireless2_config.au_audiolen = SBC_ENC_LEN_PER_FREME - CRC_PACKSUB;
#else
	s_wireless2_config.au_audiolen = SBC_DEC_LEN_PER_FREME - CRC_PACKSUB;
#endif
	MVWIRE2_ParamInit(&s_wireless2_config);

	Audio_Check1stFrameAllRight_cb = Audio_Check1stFrameAllRight;
	Audio_Check1stFrameAllRightStateGet_cb = Audio_Check1stFrameAllRightStateGet;
	Audio_Check1stFrameAllRightCounterStart_cb = Audio_Check1stFrameAllRightCounterStart;
#if defined(BOOT_APP_WIRELESS_ROLE_TX) && (BOOT_APP_WIRELESS_ROLE_TX == 1)
	{
		extern void wireless_AudioParityCntStart(void);
		extern void wireless_AudioParityCntProc(void);
		wireless_AudioParityCntStart_cb = wireless_AudioParityCntStart;
		wireless_AudioParityCntProc_cb = wireless_AudioParityCntProc;
	}
#endif

	memset((uint8_t *)BB_EM_MAP_ADDR, 0, BB_EM_SIZE);
	/*
	 * Wireless_common_init() ALONE brings up the RF. It internally runs the
	 * init-func pointer that WIRELESS_FUNCTION()/wireless2_2_X_initfuncset()
	 * registered just above -> Swbb2t1r_MasterInit()/SlaverInit(): power up
	 * RF, load BB, register the BT-isr callback and build the sync table
	 * (the "Table:/m_sync_word" (RX) / "s0_fixed_word" (TX) print).
	 *
	 * Do NOT call MVWIRE2_Init() here. The official unified SDK has ZERO
	 * MVWIRE2_Init() call-sites in every .c (verified). MVWIRE2_Init() is a
	 * thin wrapper that re-invokes that SAME init-func pointer (see its
	 * objdump: lwi.gp $r0,[gp-77936]; jral5 $r0). Calling it after
	 * common_init re-inits the RF state machine a 2nd time -> sync table
	 * printed TWICE, master never scans / slave never advertises
	 * (s_pair_st=0, dev id=-1), so TX and RX never connect.
	 */
	Wireless_common_init(&params);
	/* Mirror official WirelessInit(): non deep-sleep, disable remote sleep cmd. */
	wireless2_Enable_Remote_Sleep_Cmd(0);
	MvWire_TransBufInit();
	syncdevice2_thold = 1;

	/*
	 * The whole 2T1R state machine (sync / pairing / connect) runs in
	 * rwbt_isr -> MVWIRE2_irq -> bt_irq_2t1r_*, i.e. inside BT_IRQn.
	 * The stack library never touches NVIC itself, so the line has to be
	 * unmasked here or both sides stay silent.
	 */
	NVIC_DisableIRQ(BT_IRQn);
	NVIC_SetPriority(BT_IRQn, 0);
	NVIC_EnableIRQ(BT_IRQn);
	DBG("[WL] MvWire stack init role=%s BT_IRQ enabled\n",
	    role_tx ? "TX/Slave" : "RX/Master");
	return 0;
}

#endif /* BOOT_APP_MVWIRE_EN */
