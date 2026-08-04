/**
 * @file  app_upgrade.h
 * @brief Firmware upgrade engine — USB CDC only (bootloader).
 */
#ifndef __APP_UPGRADE_H__
#define __APP_UPGRADE_H__

#include <stdint.h>
#include "dual_partition.h"

#define UPG_CH_CDC  0

typedef struct {
    uint16_t (*rx_read)     (uint8_t *buf, uint16_t maxLen);
    void     (*tx_write)    (const uint8_t *buf, uint16_t len);
    int      (*rx_available)(void);
    uint8_t  id;
} UpgradeChannel_t;

void App_Upgrade_Init(void);
void App_Upgrade_ProcessChannel(const UpgradeChannel_t *ch);
int  App_Upgrade_IsActive(void);
int  App_Upgrade_IsFinished(void);
void App_Upgrade_InjectRaw(uint8_t ch_id, const uint8_t *buf, uint16_t len,
                           void (*tx_fn)(const uint8_t *data, uint16_t len));

#endif /* __APP_UPGRADE_H__ */
