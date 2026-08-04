/**
 * @file  app_config.h
 * @brief Bootloader minimal config for USB CDC upgrade only.
 */
#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#define CFG_APP_CONFIG

/* USB mode: CDC_ONLY (=12 in otg_device_standard_request.h) */
#define CFG_PARA_USB_MODE   12

#define DEBUG_LOG_EN

#define CFG_PARA_SAMPLE_RATE    44100
#define AUDIO_MAX_VOLUME        0x7FFF

#endif /* __APP_CONFIG_H__ */
