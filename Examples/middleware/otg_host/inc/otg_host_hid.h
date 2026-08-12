/**
 *****************************************************************************
 * @file     otg_host_hid.h
 * @author   Shanks
 * @version  V1.0.0
 * @date     2024.1.11
 * @brief    host audio V1.0 module driver interface
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2024 MVSilicon </center></h2>
 */
#ifndef __OTG_HOST_HID_H__
#define __OTG_HOST_HID_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include <string.h>
#include "type.h"
#include "otg_host_hcd.h"

#define MAX_NUM_HID_INTERFACE        5U

typedef struct
{
	PIPE_INFO            Pipe;
	uint16_t             interface;				//接口
	uint8_t              AltSettings;			//接口设置
	uint8_t              bDescriptorType;		//描述符类型
	uint16_t             wDescriptorLength;		//描述符长度
	uint8_t              supported;				//是否支持
}
HID_InterfaceControlPropTypeDef;

typedef struct
{
	PIPE_INFO Pipe;
	uint8_t supported;				//是否支持
	uint8_t	*Descriptor;			//报告描述符
	uint16_t wDescriptorLength;		//描述符长度
	uint16_t HidInterfaceNum;

	HID_InterfaceControlPropTypeDef   Interface[MAX_NUM_HID_INTERFACE];
}
HID_ClassSpecificDescTypedef;

USBH_StatusTypeDef USBH_HidSetIdle(uint16_t interface);
USBH_StatusTypeDef USBH_HidGetReport(uint16_t interface, uint8_t *buf,uint16_t size);
USBH_StatusTypeDef USBH_HidDescriptorParse(HID_ClassSpecificDescTypedef *Hid_Handle,uint8_t NumInterfaces, uint8_t *pBuf,uint8_t len);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif //__UDISK_H__

