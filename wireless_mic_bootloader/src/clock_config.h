/**
 * @file  clock_config.h
 * @brief Bootloader minimal clock_config — does NOT include app_config.h
 *        (avoids circular dependency). Provides only clock constants.
 */

#ifndef _POWER_CONFIG_H__
#define _POWER_CONFIG_H__

/* Clock modes */
#define PLL_CLK_MODE           0
#define APLL_CLK_MODE          1
#define SYSTEM_CLK_MODE        2

/* Core clock settings for bootloader */
#define SYS_CORE_SET_MODE      3  /* CORE_USER_MODE */

#define SYS_CRYSTAL_FREQ       24*1000*1000

#define SYS_CORE_APLL_FREQ     240*1000  /* kHz */
#define SYS_CORE_DPLL_FREQ     360*1000  /* kHz */

#define SYS_CORE_CLK_SELECT    PLL_CLK_MODE
#define SYS_UART_CLK_SELECT    APLL_CLK_MODE
#define SYS_USB_CLK_SELECT     APLL_CLK_MODE
#define SYS_SPDIF_CLK_SELECT   APLL_CLK_MODE

/* Flash clock */
#define FSHC_PLL_CLK_MODE      0
#define FSHC_APLL_CLK_MODE     1
#define SYS_FLASH_CLK_SELECT   FSHC_PLL_CLK_MODE
#define SYS_FLASH_FREQ_SELECT  ((SYS_CORE_DPLL_FREQ/4)*1000)

/* Audio clock */
#define SYS_AUDIO_CLK_SELECT   APLL_CLK_MODE

#endif /* _POWER_CONFIG_H__ */
