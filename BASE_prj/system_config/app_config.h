#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

#include "chip_config.h"
#include "flash_config.h"
#include "clock_config.h"

/* Turnkey 配置选择 */
#define WIRELESS_TURNKEY2_6

/* TCM 使能 */
#define TCM_EN

/* 串口打印使能 */
#define DEBUG_LOG_EN
#define CFG_UART_TX_PORT     DEBUG_TX_A10
#define CFG_UART_BANDRATE    DEBUG_BAUDRATE_115200

/* 音频/无线帧参数 (最小配置) */
#define SAMPLE_RATE          44100
#define ONE_FRAME            128
#define PACKET_AUDIO_CH      1
#define RFPACK_NAUDIO        1
#define AUDIO_QUALITY        20
#define CRC_PACKSUB          0

/* 无线模式配置 */
#define MV_WIRELESS2_MODE2
#define MV_WIRELESS2_PARAM1  0
#define MV_WIRELESS2_PARAM2  8
#define WIRELESS_FUNCTION    wireless2_2_X_initfuncset
#define TURNKEY_TAG          2_x
#define TURNKEY_NAME         "2_6"

/* 接收 FIFO 阈值 */
#define WIRELESS_RECV_FIFO_THRHLD  (RFPACK_NAUDIO + 0)

/* Link key */
#define WIRELESS_LINK_KEY0   (0x21)
#define WIRELESS_LINK_KEY1   (0x56)

/* Packet 长度 */
#define PACKET_CRC_LEN       (2)
#define PACKET_CNT_LEN       (2)

/* 编解码配置 */
#define DECODE_CH            PACKET_AUDIO_CH
#define DECODE_QUALITY       AUDIO_QUALITY
#define ENCODE_CH            0

/* 角色: master */
#define WIRELESS_SDK_ROLE    MVWIRE2_MASTER_ROLE
#define ROLE_TAG             master

/* 多项式阶数 */
#define POLYNOMIAL_ORDER     2

/* 音量配置 */
#define CFG_PARA_MAX_VOLUME_NUM      (16)
#define CFG_PARA_SYS_VOLUME_DEFAULT  (16)

/* FLASH_BOOT_EN is owned by flash_config.h (0 for bootloader APP). */

/* Started by BanAirBundy USB CDC bootloader @ 0x0 — skip Chip_Init/PLL in main. */
#define HAS_BOOTLOADER           1

/*
 * USB mode (this chip OtgEPInit / otg_fifo.c: only EP1+EP2 FIFOs):
 *   CDC_ONLY(12)     — CDC only（已通过）
 *   AUDIO_CDC(9)     — Speaker(0x02)+CDC(0x81/0x01/0x82) — 与 FIFO 匹配
 *   AUDIO_MIC_CDC(11)— 需 Mic ISO 0x84，无 EP4 FIFO；日志见 dtr=0 且声卡不可用
 */
#ifndef BOOT_APP_USB_MODE
#define BOOT_APP_USB_MODE        AUDIO_CDC
#endif
#ifndef CFG_PARA_USB_MODE
#define CFG_PARA_USB_MODE        BOOT_APP_USB_MODE
#endif
#ifndef CFG_PARA_SAMPLE_RATE
#define CFG_PARA_SAMPLE_RATE     SAMPLE_RATE
#endif
#ifndef AUDIO_MAX_VOLUME
#define AUDIO_MAX_VOLUME         4096
#endif

/* 低功耗 (可选关闭) */
/* #define CFG_LOW_POWER_MODE */

/* 电压配置 */
#define CFG_VDD3V3_3V

/* Flash 地址配置 (APP 镜像内偏移，勿与 Part A 基址混淆) */
#define CONINF_FLASH_ADDR        (0x60000)
#define CONINF_CAP_FLASH_ADDR    (0)
#define CONINF_CAP_FLASH_ADDR1   (0)

#endif /* _APP_CONFIG_H_ */
