#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

#include "chip_config.h"
#include "flash_config.h"
#include "clock_config.h"

/* Turnkey 閰嶇疆閫夋嫨 */
#define WIRELESS_TURNKEY2_6

/* TCM 浣胯兘 */
#define TCM_EN

/* 涓插彛鎵撳嵃浣胯兘 */
#define DEBUG_LOG_EN
#define CFG_UART_TX_PORT     DEBUG_TX_A10
#define CFG_UART_BANDRATE    DEBUG_BAUDRATE_115200

/* 闊抽/鏃犵嚎甯у弬鏁�(鏈�皬閰嶇疆) */
#define SAMPLE_RATE          44100
#define ONE_FRAME            128
#define PACKET_AUDIO_CH      1
#define RFPACK_NAUDIO        1
#define AUDIO_QUALITY        20
#define CRC_PACKSUB          0

/* 鏃犵嚎妯″紡閰嶇疆 */
#define MV_WIRELESS2_MODE2
#define MV_WIRELESS2_PARAM1  0
#define MV_WIRELESS2_PARAM2  8
#define WIRELESS_FUNCTION    wireless2_2_X_initfuncset
#define TURNKEY_TAG          2_x
#define TURNKEY_NAME         "2_6"

/* 鎺ユ敹 FIFO 闃堝� */
#define WIRELESS_RECV_FIFO_THRHLD  (RFPACK_NAUDIO + 0)

/* Link key */
#define WIRELESS_LINK_KEY0   (0x21)
#define WIRELESS_LINK_KEY1   (0x56)

/* Packet 闀垮害 */
#define PACKET_CRC_LEN       (2)
#define PACKET_CNT_LEN       (2)

/*
 * wireless_lib 鎺ュ叆寮�叧锛�
 *   BOOT_APP_WIRELESS_EN=1 缂栬瘧骞跺惎鍔ㄦ棤绾夸换鍔�
 *   BOOT_APP_WIRELESS_ROLE_TX=0 RX/Master锛�1 TX/Slave
 *   BOOT_APP_MVWIRE_EN=1 閾炬帴 libwireless2 + Association + SBC
 */
/*
 * Wireless is selected here, not by the IDE -D list.
 * EN: 0 = off, 1 = on.
 * 0 = RX/Master, 1 = TX/Slave.
 */
#ifdef BOOT_APP_WIRELESS_EN
#undef BOOT_APP_WIRELESS_EN
#endif
#define BOOT_APP_WIRELESS_EN         1

#ifdef BOOT_APP_WIRELESS_ROLE_TX
#undef BOOT_APP_WIRELESS_ROLE_TX
#endif
#define BOOT_APP_WIRELESS_ROLE_TX    0

#ifdef BOOT_APP_MVWIRE_EN
#undef BOOT_APP_MVWIRE_EN
#endif
#define BOOT_APP_MVWIRE_EN           BOOT_APP_WIRELESS_EN

/*
 * Bare-metal build (no FreeRTOS scheduler): wireless_lib dynamic allocations
 * (SBC ctx) come from the T_Heap allocator (mv_utils/heap.c) via T_PortMalloc.
 * T_HeapInit() runs in main() before Wireless_Init(). libc malloc() is NOT
 * usable here (no _sbrk -> returns NULL -> Wireless_Init() fails with -2).
 */
#ifndef WL_USE_FREERTOS_HEAP
#define WL_USE_FREERTOS_HEAP         0
#endif

/* 缂栬В鐮侀�閬擄細涓�1532 Turnkey 瑙掕壊闀滃儚 */
#ifdef ENCODE_CH
#undef ENCODE_CH
#endif
#ifdef DECODE_CH
#undef DECODE_CH
#endif
#ifdef WIRELESS_SDK_ROLE
#undef WIRELESS_SDK_ROLE
#endif
#ifdef ROLE_TAG
#undef ROLE_TAG
#endif

#define ENCODE_QUALITY       AUDIO_QUALITY
#define DECODE_QUALITY       AUDIO_QUALITY
#if BOOT_APP_WIRELESS_ROLE_TX
#define ENCODE_CH            PACKET_AUDIO_CH
#define DECODE_CH            0
#undef WIRELESS_RECV_FIFO_THRHLD
#define WIRELESS_SDK_ROLE    2 /* MVWIRE2_SLAVER_ROLE */
#define ROLE_TAG             slaver
#else
#define ENCODE_CH            0
#define DECODE_CH            PACKET_AUDIO_CH
#define WIRELESS_SDK_ROLE    1 /* MVWIRE2_MASTER_ROLE */
#define ROLE_TAG             master
#endif

/* 澶氶」寮忛樁鏁�*/
#define POLYNOMIAL_ORDER     2

/* 闊抽噺閰嶇疆 */
#define CFG_PARA_MAX_VOLUME_NUM      (16)
#define CFG_PARA_SYS_VOLUME_DEFAULT  (16)

/* FLASH_BOOT_EN is owned by flash_config.h (0 for bootloader APP). */

/* Started by BanAirBundy USB CDC bootloader @ 0x0 鈥�skip Chip_Init/PLL in main. */
#define HAS_BOOTLOADER           1

/*
 * USB mode (this chip OtgEPInit / otg_fifo.c: only EP1+EP2 FIFOs):
 *   CDC_ONLY(12)     鈥�CDC only锛堝凡閫氳繃锛�
 *   AUDIO_CDC(9)     鈥�Speaker(0x02)+CDC(0x81/0x01/0x82) 鈥�涓�FIFO 鍖归厤
 *   AUDIO_MIC_CDC(11)鈥�闇�Mic ISO 0x84锛屾棤 EP4 FIFO锛涙棩蹇楄 dtr=0 涓斿０鍗′笉鍙敤
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

/* 浣庡姛鑰�(鍙�鍏抽棴) */
/* #define CFG_LOW_POWER_MODE */

/* 鐢靛帇閰嶇疆 */
#define CFG_VDD3V3_3V

/* Flash 鍦板潃閰嶇疆 (APP 闀滃儚鍐呭亸绉伙紝鍕夸笌 Part A 鍩哄潃娣锋穯) */
#define CONINF_FLASH_ADDR        (0x60000)
#define CONINF_CAP_FLASH_ADDR    (0)
#define CONINF_CAP_FLASH_ADDR1   (0)

#endif /* _APP_CONFIG_H_ */
