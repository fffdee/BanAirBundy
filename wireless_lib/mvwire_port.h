/**
 * @file mvwire_port.h
 * @brief MVWIRE2 / AudioAssociation port for boot_app (Turnkey 2_6)
 */
#ifndef __MVWIRE_PORT_H__
#define __MVWIRE_PORT_H__

#include <stdint.h>
#include "type.h"

#ifndef BOOT_APP_MVWIRE_EN
#define BOOT_APP_MVWIRE_EN  1
#endif

#if BOOT_APP_MVWIRE_EN

#ifdef __cplusplus
extern "C" {
#endif

/** RF + role + BB init (mirrors 1532 WirelessInit sequence). */
int MvWire_StackInit(uint8_t role_tx);

/** Mark audio path ready so ConnectedCB can enter CONNECT_AUDIO. */
void MvWire_AudioReadySet(uint8_t ready);

/** Conn-state display (call from Wireless_Schedule). */
void MvWire_StackSchedule(void);

/** TX: PackBuild for Turnkey 2_6 (Cnt+Cmd+CRC16). */
void MvWire_PackBuild(uint8_t *frame, uint8_t cnt, uint8_t cmd);

/** TX FIFO helpers */
void MvWire_TransBufInit(void);
uint16_t MvWire_TransSpaceLen(void);
void MvWire_TransBufWrite(const uint8_t *data, unsigned int len);

/** RX: AudioAssociation init / one-frame decode. */
int MvWire_AssocInit(void);
uint16_t MvWire_AssocProcess(int16_t *pcm_l, int16_t *pcm_r);

/** Map device ConStatus into wireless_lib ConnectStatus_t values. */
uint8_t MvWire_GetDeviceStatus(uint8_t device_index);

/** 1st-frame sync state per device: 0=none, 1=counting, 2=synced.
 *  Mirrors official Audio_Check1stFrameAllRightStateGet(); RX uses it to prime
 *  the DAC ring on first lock (audio_main.c:543-583 equivalent). */
uint8_t MvWire_1stFrameSyncState(uint8_t id);

/** App-level connect/disconnect hooks (from Wireless_Register*Cb). */
void MvWire_RegisterAppConnCb(void (*conn)(uint8_t), void (*disc)(uint8_t));

#ifdef __cplusplus
}
#endif

#endif /* BOOT_APP_MVWIRE_EN */

#endif /* __MVWIRE_PORT_H__ */
