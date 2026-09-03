/**
 * @file mvwire_assoc.c
 * @brief RX AudioAssociation init + process (Turnkey 2_6 mono)
 */
#include "mvwire_port.h"

#if BOOT_APP_MVWIRE_EN && !(defined(BOOT_APP_WIRELESS_ROLE_TX) && (BOOT_APP_WIRELESS_ROLE_TX == 1))

#include "app_config.h"
#include "sbc_encoder.h"
#include "audio_association.h"
#include "debug.h"
#include <string.h>

#if defined(DECODE_CH) && (DECODE_CH != 0)

static uint8_t s_silence[] = SBC_SIL_CH_FRAME_SAMPLERATE_QUALITY;
static uint8_t s_sbc_buf[SBC_DEC_LEN_PER_FREME * 2 + 6];
static short s_pcm_l[SBC_DEC_LEN_PER_FREME * 2 * 20];
static short s_pcm_r[SBC_DEC_LEN_PER_FREME * 2 * 20];
static Rx_AudioAssociation_param_t s_assoc;

extern void Audio_Check1stFrameAllRightCounterResetForAudioAssociation(uint8_t id);

int MvWire_AssocInit(void)
{
	memset(&s_assoc, 0, sizeof(s_assoc));
	s_assoc.FrameGroups = STEREO_TWO_DEVICE;
	s_assoc.SilencePack = s_silence;
	s_assoc.wireless_frames = WIRELESS_RECV_FIFO_THRHLD;
	s_assoc.AddHeaderLen = CRC_PACKSUB;
	s_assoc.pack_info_len = PACKET_CNT_LEN;
	s_assoc.frame_len = ONE_FRAME;
	s_assoc.Encoder_out_len = SBC_DEC_LEN_PER_FREME;
	s_assoc.sbc_buf_p = s_sbc_buf;
	s_assoc.MicLeftPcmFifo_p = s_pcm_l;
	s_assoc.MicRightPcmFifo_p = s_pcm_r;
	s_assoc.LogEnable = 0;
	s_assoc.MicLeftPcmFifo_len_set = sizeof(s_pcm_l);
	s_assoc.MicRightPcmFifo_len_set = sizeof(s_pcm_r);
	s_assoc.Wireless2_2T1RDelaySyncResetFunc_t =
		Audio_Check1stFrameAllRightCounterResetForAudioAssociation;

	AudioPlc128Init();
	AudioAssociationInit(&s_assoc);
	DBG("[WL] AudioAssociation init ok\n");
	return 0;
}

uint16_t MvWire_AssocProcess(int16_t *pcm_l, int16_t *pcm_r)
{
	if (!pcm_l)
		return 0;
	return AudioAssociationProcess(pcm_l, pcm_r);
}

#else

int MvWire_AssocInit(void) { return -1; }
uint16_t MvWire_AssocProcess(int16_t *pcm_l, int16_t *pcm_r)
{
	(void)pcm_l;
	(void)pcm_r;
	return 0;
}

#endif /* DECODE_CH */

#else /* TX role or MVWIRE off */

#if BOOT_APP_MVWIRE_EN
int MvWire_AssocInit(void) { return 0; }
uint16_t MvWire_AssocProcess(int16_t *pcm_l, int16_t *pcm_r)
{
	(void)pcm_l;
	(void)pcm_r;
	return 0;
}
#endif

#endif
