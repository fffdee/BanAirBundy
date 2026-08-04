/**
 * @file  cdc_upgrade.h
 * @brief USB CDC firmware upgrade bridge (bootloader).
 */
#ifndef __CDC_UPGRADE_H__
#define __CDC_UPGRADE_H__

#ifdef __cplusplus
extern "C" {
#endif

void CDC_Upgrade_Init(void);
void CDC_Upgrade_EnterMode(void);
int  CDC_Upgrade_InMode(void);
int  CDC_Upgrade_CheckEnter(void);
void CDC_Upgrade_Process(void);
int  CDC_Upgrade_IsActive(void);

#ifdef __cplusplus
}
#endif

#endif /* __CDC_UPGRADE_H__ */
