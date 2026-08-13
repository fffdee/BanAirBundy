/**
 * @file mvwire_transbuf.c
 * @brief TX RF send FIFO + MODE2 ready API (from 1532 audio_main.c)
 */
#include "mvwire_port.h"

#if BOOT_APP_MVWIRE_EN

#include "type.h"
#include "app_config.h"
#include "sbc_encoder.h"
#include "mcu_circular_buf.h"
#include "rom.h"
#include "irqn.h"
#include "wireless2.h"
#include <string.h>

#ifndef RFAUDIO_TRANS_LEN
#define RFAUDIO_TRANS_LEN  RFAUDIO_TRANS_A
#endif

static MCU_CIRCULAR_CONTEXT s_tx_circ;
static uint8_t s_tx_fifo[RFAUDIO_TRANS_A * 8];
static unsigned int s_audio_ignorecnt;
static unsigned char s_audio_conuter;
static unsigned char s_audio_paritycnt = 0xff;
static unsigned char s_audio_rfstartcnt;

void MvWire_TransBufInit(void)
{
	GIE_DISABLE();
	MCUCircular_Config(&s_tx_circ, s_tx_fifo, sizeof(s_tx_fifo));
	s_audio_conuter = 0;
	GIE_ENABLE();
}

uint16_t MvWire_TransSpaceLen(void)
{
	return (uint16_t)MCUCircular_GetSpaceLen(&s_tx_circ);
}

void MvWire_TransBufWrite(const uint8_t *data, unsigned int len)
{
	GIE_DISABLE();
	MCUCircular_PutData(&s_tx_circ, (uint8_t *)data, len);
	GIE_ENABLE();
}

/* --- symbols required by libwireless2.a --- */

void Wireless_TransBufInit(void)
{
	MvWire_TransBufInit();
}

unsigned int Wireless_TransBufRead(unsigned char *data, unsigned int len)
{
	return MCUCircular_GetData(&s_tx_circ, data, len);
}

unsigned int wireless_AudioIgnorecntGet(void)
{
	return s_audio_ignorecnt;
}

void wireless_AudioIgnorecntSet(unsigned int ignore_cnt)
{
	s_audio_ignorecnt = ignore_cnt;
}

unsigned char wireless_AudioParityCntGet(void)
{
	return s_audio_paritycnt;
}

void wireless_AudioParityCntReset(void)
{
	s_audio_paritycnt = 0xff;
	s_audio_rfstartcnt = 0;
}

void wireless_AudioParityCntStart(void)
{
	extern unsigned char audio_init_isready;
	if (s_audio_rfstartcnt >= 5) {
		if (s_audio_paritycnt == 0xff) {
			s_audio_paritycnt = 0;
			MvWire_TransBufInit();
		}
	} else if (audio_init_isready == 1) {
		s_audio_rfstartcnt++;
	}
}

void wireless_AudioParityCntProc(void)
{
	if (s_audio_paritycnt != 0xff)
		s_audio_paritycnt++;
}

static unsigned char wireless_AudioCounterProcess(void)
{
	unsigned char audio_cntsend = (unsigned char)(RFPACK_NAUDIO + 1);

	if (s_audio_ignorecnt != 0) {
		MvWire_TransBufInit();
		s_audio_ignorecnt--;
		return 0;
	}

	if ((s_audio_conuter == 0) && (wireless_AudioParityCntGet() != 0xff)) {
		if (MCUCircular_GetDataLen(&s_tx_circ) >= RFAUDIO_TRANS_LEN)
			s_audio_conuter = audio_cntsend;
	} else if (s_audio_conuter == 0xff) {
		if (MCUCircular_GetDataLen(&s_tx_circ) >= RFAUDIO_TRANS_LEN)
			s_audio_conuter = audio_cntsend;
	} else if (s_audio_conuter != 0) {
		s_audio_conuter++;
		if (s_audio_conuter > audio_cntsend) {
			if (MCUCircular_GetDataLen(&s_tx_circ) < RFAUDIO_TRANS_LEN) {
				wireless_AudioIgnorecntSet(300);
				MvWire_TransBufInit();
			}
			s_audio_conuter = 1;
			return 1;
		}
	}
	return 0;
}

#ifdef MV_WIRELESS2_MODE2
uint16_t Wireless_RfTransBufLen(void)
{
	if (wireless_AudioCounterProcess() == 0)
		return 0;
	return (uint16_t)MCUCircular_GetDataLen(&s_tx_circ);
}
#else
bool Wireless_TransPacketIsReady(uint8_t StepNum)
{
	(void)StepNum;
	return MCUCircular_GetDataLen(&s_tx_circ) >= RFAUDIO_TRANS_LEN;
}
#endif

void MvWire_PackBuild(uint8_t *frame, uint8_t cnt, uint8_t cmd)
{
	unsigned short sbc_crc16;

	frame[RFAUDIO_FRAME_LEN + 0] = cnt;
	frame[RFAUDIO_FRAME_LEN + 1] = cmd;
	sbc_crc16 = ROM_CRC16((char *)frame + CRC_PACKSUB,
			      RFAUDIO_FRAME_LEN - CRC_PACKSUB + PACKET_CNT_LEN, 0);
	frame[RFAUDIO_FRAME_LEN + PACKET_CNT_LEN + 0] = (uint8_t)(sbc_crc16 & 0xff);
	frame[RFAUDIO_FRAME_LEN + PACKET_CNT_LEN + 1] = (uint8_t)(sbc_crc16 >> 8);
}

/* RX/lib may reference these; provide empty stubs for 2_6 unidirectional. */
void audio_PackAddheader(unsigned char *audio_encodeframe)
{
	(void)audio_encodeframe;
}

void audio_Pack2Frames(unsigned char *audio_encodeframe)
{
	(void)audio_encodeframe;
}

#endif /* BOOT_APP_MVWIRE_EN */
