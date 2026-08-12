/**
 * @file  usb_identity.h
 * @brief Bootloader USB identity protocol (VID/PID).
 *
 * Host tools identify the BG boot family and product platform solely by
 * the USB device descriptor:
 *
 *   PID = 0x4247 ('B''G')  — BG Bootloader family
 *   VID = product code:
 *         0x0001  BanBox
 *         0x0002  BanAirBundy
 *
 * Example (this product): VID=0x0002 PID=0x4247 → BanAirBundy Bootloader
 */
#ifndef __USB_IDENTITY_H__
#define __USB_IDENTITY_H__

#define BG_USB_PID              0x4247u

#define BG_USB_VID_BANBOX       0x0001u
#define BG_USB_VID_BANAIRBUNDY  0x0002u

/* This board / product */
#ifndef BL_USB_VID
#define BL_USB_VID              BG_USB_VID_BANAIRBUNDY
#endif

#ifndef BL_USB_PID
#define BL_USB_PID              BG_USB_PID
#endif

#endif /* __USB_IDENTITY_H__ */
