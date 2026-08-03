/**
 * @file  app_config.h
 * @brief Bootloader minimal app_config — provides only the macros
 *        required by USB CDC driver headers and FreeRTOS.
 *
 * This header OVERRIDES the full app_config.h from unified_sdk.
 * The bootloader src/ directory is placed first in the -I search path,
 * so the compiler finds this lightweight version instead.
 *
 * DO NOT include the full app_config.h — it pulls in wireless, audio,
 * and other subsystems that the bootloader does not need.
 */

#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

/* Chip selection — same as the full app_config.h */
#include "chip_config.h"

/* Flash layout — bootloader minimal version */
#include "flash_config.h"

/* Device role: bootloader is not TX or RX, but USB macros need one defined.
 * We define TX as default since bootloader doesn't care. */
#ifndef WIRELESS_DEVICE_TX
#define WIRELESS_DEVICE_TX
#endif

/* ── USB mode — CDC_ONLY for bootloader (no audio composite) ── */
#define CDC_ONLY         7
#define AUDIO_MIC_CDC    8
#define CFG_PARA_USB_MODE   CDC_ONLY

/* ── Debug log — bootloader always enables debug output ── */
#define DEBUG_LOG_EN

/* ── Turnkey mode — must define one for wireless headers if included ── */
#define WIRELESS_TURNKEY2_6
#define SAMPLE_RATE         44100
#define ONE_FRAME           128
#define PACKET_AUDIO_CH     1
#define RFPACK_NAUDIO       1
#define AUDIO_QUALITY       20
#define CRC_PACKSUB         0

/* ── Minimal defines needed by USB driver ── */
#define TCM_EN
#define PACKET_CRC_LEN      2
#define PACKET_CNT_LEN      2

#define CFG_PARA_SAMPLE_RATE    44100
#define AUDIO_MAX_VOLUME        0x7FFF

/* TX/RX role mapping (bootloader doesn't use these but headers reference them) */
#define ENCODE_CH           PACKET_AUDIO_CH
#define DECODE_CH           0
#define ENCODE_QUALITY      AUDIO_QUALITY
#define MVWIRE2_MASTER_ROLE 1
#define MVWIRE2_SLAVER_ROLE 0
#define WIRELESS_SDK_ROLE   MVWIRE2_SLAVER_ROLE

/* ── FreeRTOS config — needed by delay.h and heap ── */
#define configUSE_PREEMPTION            1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

#endif /* __APP_CONFIG_H__ */
