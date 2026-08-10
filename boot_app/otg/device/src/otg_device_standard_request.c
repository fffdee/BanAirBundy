/**
 *****************************************************************************
 * @file     device_stor_audio_request.c
 * @author   owen
 * @version  V1.0.0
 * @date     7-September-2015
 * @brief    device audio and mass-storage module driver interface
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */

#include <string.h>
#include "type.h"
#include "debug.h"
#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
#include "otg_device_descriptor.h"
#include "otg_device_audio.h"
#include "otg_device_cdc.h"
#include "usb_audio_api.h"

#ifdef CFG_APP_CONFIG
#include "app_config.h"
#endif

/* boot_app: keep CFG_APP_USB_AUDIO_MODE_EN from usb_audio_api.h for composite */

//------------------------------------//
// HID USB????????? - ????PC Tool??????????????????void HIDUsb_Tx(uint8_t *buf, uint16_t len);


void HIDUsb_Rx(uint8_t *buf, uint16_t len);



uint8_t hid_tx_buf[256];
void IsAndroid(void);

//------------------------------------//


const uint8_t  DeviceQualifier[10] = {10,6,0x10,0x01,0,0,0,64,1,0};
extern void OnDeviceAudioRcvIsoPacket(void);
extern void OnDeviceAudioSendIsoPacket(void);

extern void OTG_DeviceAudioRequest(void);
void hid_recive_data(void);
void hid_send_data(void);
const uint8_t DeviceString_LangID[] = {0x04, 0x03, 0x09, 0x04};

uint8_t Setup[8];
uint8_t Request[256];

uint8_t *ConfigDescriptor;
uint8_t *InterfaceNum;

// #region agent log
#define USB_DBG_SETUP_MAX 8
typedef struct {
	uint8_t setup[8];
} UsbDbgSetup_t;
static volatile uint32_t s_dbg_bus_reset;
static volatile uint32_t s_dbg_setup_ok;
static volatile uint32_t s_dbg_setup_err;
static volatile uint32_t s_dbg_setup_short;
static volatile uint32_t s_dbg_last_setup_err;
static volatile uint32_t s_dbg_last_bus_event;
static volatile uint32_t s_dbg_first_reset_tick;
static volatile uint32_t s_dbg_first_ok_tick;
static UsbDbgSetup_t s_dbg_setup[USB_DBG_SETUP_MAX];
extern unsigned int GetSysTick1MsCnt(void);

static void OTG_DeviceDebugHwSnapshot(uint32_t *usb_div_minus1,
				      uint32_t *usb_mux,
				      uint32_t *dp_pwr,
				      uint32_t *ep0_csr)
{
	/* Clock_USBClkDivSet stores (Div-1) at 0x40021010 */
	*usb_div_minus1 = *(volatile uint32_t *)0x40021010UL;
	/* Clock_USBClkSelect: 0x4002103C bit11 — 0=DPLL, 1=APLL */
	*usb_mux = (uint32_t)(*(volatile uint16_t *)0x4002103CUL);
	/* DP pull-up / power control live near 0x40000194 (see PortEnableDPPullUp) */
	*dp_pwr = *(volatile uint32_t *)0x40000194UL;
	/* EP0 CSR: RxPktRdy = bit0 @ 0x40000011 */
	*ep0_csr = (uint32_t)(*(volatile uint8_t *)0x40000011UL);
}
// #endregion agent log

const char *gDeviceProductString ="BG Card audio";		//max length: 32bytes
const char *gDeviceString_Manu ="BanGO";		//max length: 32bytes
const char *gDeviceString_SerialNumber ="20250405";//max length: 32bytes
uint8_t *gDeviceString_Index;

extern UsbAudio UsbAudioSpeaker;
extern UsbAudio UsbAudioMic;

void OTG_DeviceModeSel(uint8_t Mode,uint16_t UsbVid,uint16_t UsbPid)
{
	
	DeviceDescriptor[8] = UsbVid;
	DeviceDescriptor[9] = UsbVid>>8;
	DeviceDescriptor[10] = UsbPid;
	DeviceDescriptor[11] = UsbPid>>8;
	ConfigDescriptor = (uint8_t *)ConfigDescriptorTab[Mode];
	InterfaceNum = (uint8_t *)InterFaceNumTab[Mode];

	/* Windows: EF/02/01 + IAD required for CDC ACM (usbser.sys) */
	DeviceDescriptor[4] = 0xEF;
	DeviceDescriptor[5] = 0x02;
	DeviceDescriptor[6] = 0x01;

	if (Mode == CDC_ONLY) {
		gDeviceProductString = "BG Bootloader";
	} else {
		gDeviceProductString = "BG Card audio";
	}

 	gDeviceString_Manu 		        = "BanGO";
	gDeviceString_SerialNumber      = "20250405";
}



/**
 * @brief  ?????????????????????????????????????????????????????? * @param  Resp ???????????????????
 * @param  n ???????????????????????????? or 2
 * @return NONE
 */
void OTG_DeviceSendResp(uint16_t Resp, uint8_t n)
{
	Resp = CpuToLe16(Resp);
	OTG_DeviceControlSend((uint8_t*)&Resp, n,3);
}

/**
 * @brief  ?????????????????????????????????????????? * @param  NONE
 * @return NONE
 */
void OTG_DeviceGetDescriptor(void)
{
	uint8_t 	StringBuf[32 * 2 + 2];
	uint8_t*	UsbSendPtr = 0;
	uint16_t	Len = 0;
	switch(Setup[3])
	{
		case USB_DT_DEVICE:
		//DBG("USB_DT_DEVICE\n");
			UsbSendPtr = (uint8_t*)DeviceDescriptor;
			Len = sizeof(DeviceDescriptor);
			break;

		case USB_DT_CONFIG:
		//DBG("USB_DT_CONFIG\n");
			UsbSendPtr = (uint8_t*)ConfigDescriptor;
            Len = UsbSendPtr[3];
            Len = Len<<8;
            Len = Len + UsbSendPtr[2];
			break;

		case USB_DT_STRING:
		//DBG("USB_DT_STRING\n");
			if(Setup[2] == 0)			//lang ids
			{
				UsbSendPtr = (uint8_t*)DeviceString_LangID;
				Len = UsbSendPtr[0];
				break;
			}
			else if(Setup[2] == 1)		//manu
			{
				UsbSendPtr = (uint8_t*)gDeviceString_Manu;
			}
			else if(Setup[2] == 2)		//product
			{
				UsbSendPtr = (uint8_t*)gDeviceProductString;
			}
			else if(Setup[2] == 4)		//debug effect
			{
				UsbSendPtr = gDeviceString_Index;
			}			
			else 	//serial number
			{
				UsbSendPtr = (uint8_t*)gDeviceString_SerialNumber;
			}

			for(Len = 0; Len < 32; Len++)
			{
				if(UsbSendPtr[Len] == '\0')
				{
					break;
				}
				StringBuf[2 + Len * 2 + 0] = UsbSendPtr[Len];
				StringBuf[2 + Len * 2 + 1] = 0x00;
			}

			Len = Len * 2 + 2;
			StringBuf[0] = Len;
			StringBuf[1] = 0x03;
			UsbSendPtr = StringBuf;
			break;

		case USB_DT_INTERFACE:
			//PC??????????????????????????????
		//	DBG("USB_DT_INTERFACE\n");
			break;

		case USB_DT_ENDPOINT:
			//PC??????????????????????????????
		//DBG("USB_DT_ENDPOINT\n");
			break;
			
		case USB_DT_DEVICE_QUALIFIER:
			UsbSendPtr = (uint8_t*)DeviceQualifier;
			Len = 10;
			break;

		case USB_HID_REPORT:
			if(Setup[4] == InterfaceNum[HID_DATA_INTERFACE_NUM])
			{
				//DBG("HID_DATA_INTERFACE_NUM REPORTR\n");
#if HID_DATA_FUN_EN
				UsbSendPtr = (uint8_t*)&HidDataReportDescriptor[0];
				Len = sizeof(HidDataReportDescriptor);
#endif
			}
			else if(Setup[4] == InterfaceNum[HID_CTL_INTERFACE_NUM])
			{
				//DBG("HID_CTL_INTERFACE_NUM REPORTR\n");
				UsbSendPtr = (uint8_t*)&AudioCtrlReportDescriptor[0];
				Len = sizeof(AudioCtrlReportDescriptor);
			}
			else
			{
				//DBG("NOT FOUND INTERFACE %d\n",Setup[4]);
			}
			break;

		default:
			OTG_DeviceStallSend(DEVICE_CONTROL_EP);
			return;
	}

	if(Len > (Setup[7] * 256 + Setup[6]))
	{

		Len = Setup[7] * 256 + Setup[6];
	}
	OTG_DeviceControlSend((uint8_t*)UsbSendPtr, Len, 10);
}

void OTG_DeviceAudioInit();
//extern uint32_t SpeakerRun;
void OTG_DeviceStandardRequest()
{
	uint8_t Resp[8];
	//DBG("\nSetup[1] = %d\n\n", Setup[1]);
	switch(Setup[1])
	{
		case USB_REQ_GET_STATUS:
			Resp[0] = 0x00;
			Resp[1] = 0x00;
			OTG_DeviceControlSend((uint8_t*)&Resp, 2, 10);
			break;

		case USB_REQ_CLEAR_FEATURE:
			/* Status for OUT wLength=0 ? was falling through (break stuck in comment) */
			{
				static uint8_t zlp;
				OTG_DeviceControlSend(&zlp, 0, 10);
			}
			break;

		case USB_REQ_SET_FEATURE:
			/* Do not stall EP0 ? previous StallSend(Setup[4]) was commented junk */
			{
				static uint8_t zlp;
				OTG_DeviceControlSend(&zlp, 0, 10);
			}
			break;

		case USB_REQ_SET_ADDRESS:
			OTG_DeviceAddressSet(Setup[2] & 0x7F);
			break;

		case USB_REQ_GET_DESCRIPTOR:
			//OTG_DBG("GetDescriptor\n");
			OTG_DeviceGetDescriptor();
			break;
		
		case USB_REQ_SET_DESCRIPTOR:
			//OTG_DBG("GetDescriptor111\n");
			//DeviceGetDescriptor();
			break;

		case USB_REQ_GET_CONFIGURATION:
		//	OTG_DBG("GetConfiguration\n");
			Resp[0] = 0x01;
			//OtgDeviceControlSend(Resp, 1,3);
			OTG_DeviceControlSend((uint8_t*)&Resp, 1,3);
			break;

		case USB_REQ_SET_CONFIGURATION:
			{
				static uint8_t zlp;
				/* Keep EP0 path minimal: reset EPs + status ZLP only.
				 * Do NOT CDC_Init here — defer to main after SET_CONFIG. */
				OTG_DeviceEndpointReset(DEVICE_CDC_CMD_EP, TYPE_INT_IN);
				OTG_DeviceEndpointReset(DEVICE_CDC_DATA_IN_EP, TYPE_BULK_IN);
				OTG_DeviceEndpointReset(DEVICE_CDC_DATA_OUT_EP, TYPE_BULK_OUT);
#ifdef CFG_APP_USB_AUDIO_MODE_EN
				OTG_DeviceEndpointReset(DEVICE_INT_IN_EP,TYPE_INT_IN);
				OTG_DeviceEndpointReset(DEVICE_ISO_IN_EP,TYPE_ISO_IN);
				OTG_DeviceEndpointReset(DEVICE_ISO_OUT_EP,TYPE_ISO_OUT);
				OTG_EndpointInterruptEnable(DEVICE_ISO_OUT_EP,OnDeviceAudioRcvIsoPacket);
				OTG_EndpointInterruptEnable(DEVICE_ISO_IN_EP,OnDeviceAudioSendIsoPacket);
				OTG_DeviceISOSend(DEVICE_ISO_IN_EP,0,0);
				OTG_DeviceAudioInit();
				UsbAudioMic.InitOk = 1;
				UsbAudioSpeaker.InitOk = 1;
#endif
				OTG_DeviceControlSend(&zlp, 0, 10);
				g_usb_configured = 1;
			}
			break;

		case USB_REQ_GET_INTERFACE:
			Resp[0] = 0x00;
			OTG_DeviceControlSend((uint8_t*)&Resp, 1, 10);
			break;

		case USB_REQ_SET_INTERFACE:
		#ifdef CFG_APP_USB_AUDIO_MODE_EN
			if(Setup[4] == InterfaceNum[AUDIO_SRM_IN_INTERFACE_NUM])
			{
				UsbAudioMic.AltSet = Setup[2];
			}
			else if(Setup[4] == InterfaceNum[AUDIO_SRM_OUT_INTERFACE_NUM])
			{
				UsbAudioSpeaker.AltSet = Setup[2];
			}
		#endif
			/* Control OUT wLength=0: software must finish status (ZLP + DataEnd) */
			{
				static uint8_t zlp;
				OTG_DeviceControlSend(&zlp, 0, 10);
			}
			break;

		case USB_REQ_SYNCH_FRAME:
			//OTG_DBG("SYNC FRAME\n");
			break;

		default:
		//	OTG_DBG("UsbDeviceSendStall 006\n");
			OTG_DeviceStallSend(DEVICE_CONTROL_EP);
			break;
	}
}

uint32_t pc_upgrade = 0;

void OTG_DeviceClassRequest()
{
	uint8_t bm = Setup[0];
	uint8_t req = Setup[1];
	uint8_t ifn = Setup[4];

	/* CRITICAL: never DBG before EP0 status/data ? UART ~15ms ? host stall pid.
	 * (Regression: entry log before CDC_Request re-broke SET_CTRL_LINE / LINE_CODING.) */

	/* CDC ACM by request code (works for any IF numbering) */
	if ((bm == 0x21 || bm == 0xA1) &&
	    (req == CDC_SET_LINE_CODING || req == CDC_GET_LINE_CODING ||
	     req == CDC_SET_CONTROL_LINE_STATE || req == CDC_SEND_BREAK ||
	     req == CDC_SEND_ENCAPSULATED_COMMAND || req == CDC_GET_ENCAPSULATED_RESPONSE))
	{
		OTG_DeviceCDC_Request();
		return;
	}
	/*
	 * Route by InterfaceNum table ONLY.
	 * Do NOT hardcode ifn==0/1 → CDC: that breaks AUDIO_CDC where IF0/IF1
	 * are Audio Control/Stream (Windows then fails with Unknown Device).
	 * CDC_ONLY maps CDC_CTL=0 / CDC_DATA=1 via InterFaceNum_Tab, so it still works.
	 */
	if (ifn == InterfaceNum[CDC_CTL_INTERFACE_NUM] ||
	    ifn == InterfaceNum[CDC_DATA_INTERFACE_NUM])
	{
		OTG_DeviceCDC_Request();
		return;
	}

	if ((bm == 0x22) && (req == 0x01))
	{
#ifdef CFG_APP_USB_AUDIO_MODE_EN
		OTG_DeviceAudioRequest();
#endif
		return;
	}
	if (ifn == InterfaceNum[MSC_INTERFACE_NUM])
	{
		OTG_DeviceSendResp(0x0000, 1);
		return;
	}
#ifdef CFG_APP_USB_AUDIO_MODE_EN
	if (ifn == InterfaceNum[AUDIO_ATL_INTERFACE_NUM] ||
	    ifn == InterfaceNum[AUDIO_SRM_OUT_INTERFACE_NUM] ||
	    ifn == InterfaceNum[AUDIO_SRM_IN_INTERFACE_NUM])
	{
		OTG_DeviceAudioRequest();
		return;
	}
#endif
	/* Unmatched: ACK with ZLP rather than stall EP0 (stall latches until clear) */
	{
		static uint8_t zlp;
		OTG_DeviceControlSend(&zlp, 0, 10);
	}
}


//???????????????????????????????????????????????
void OTG_DeviceManufacturerRequest()
{
	//???????????????????????????????????????????????
}


//??????????????
void OTG_DeviceOtherRequest()
{
	//OTG_DBG("UsbDeviceSendStall\n");
	OTG_DeviceStallSend(DEVICE_CONTROL_EP);
}

//__attribute__((weak))// bkd // 2019.5.7
//{
//}

/**
 * @brief  ?????????PC??????????????????????????????? * @param  NONE
 * @return NONE
 */
void OTG_DeviceRequestProcess(void)
{
	//DBG("is run");
	uint8_t BusEvent = OTG_DeviceBusEventGet();
	uint32_t DataLeng;
	uint8_t ReqType;
	OTG_DEVICE_ERR_CODE setup_err;

	// #region agent log
	s_dbg_last_bus_event = BusEvent;
	// #endregion agent log
	if(BusEvent & 0x04)
	{
		// #region agent log
		s_dbg_bus_reset++;
		if (s_dbg_first_reset_tick == 0u)
			s_dbg_first_reset_tick = GetSysTick1MsCnt();
		// #endregion agent log
		/* Match Example_USB: do NOT call AddressSet(0) here (blocked ~20ms, bit4 never set).
		 * Hardware clears address on bus reset. */
#ifdef CFG_APP_USB_AUDIO_MODE_EN
		UsbAudioMic.InitOk = 0;
		UsbAudioSpeaker.InitOk = 0;
#endif
		OTG_DeviceCDC_DeInit();
		g_usb_configured = 0;
	}
	setup_err = OTG_DeviceSetupReceive(Setup, 8, &DataLeng);
	if(setup_err != DEVICE_NONE_ERR)
	{
		// #region agent log
		s_dbg_setup_err++;
		s_dbg_last_setup_err = (uint32_t)setup_err;
		// #endregion agent log
		return;
	}
	/* Status-OUT ZLP can raise RxPktRdy with len!=8; ignore (stale Setup[]). */
	if (DataLeng != 8u) {
		// #region agent log
		s_dbg_setup_short++;
		// #endregion agent log
		return;
	}
	// #region agent log
	if (s_dbg_setup_ok < USB_DBG_SETUP_MAX) {
		memcpy(s_dbg_setup[s_dbg_setup_ok].setup, Setup, sizeof(Setup));
	}
	s_dbg_setup_ok++;
	if (s_dbg_first_ok_tick == 0u)
		s_dbg_first_ok_tick = GetSysTick1MsCnt();
	// #endregion agent log
	/* IMPORTANT: do NOT DBG before handling — SET_ADDRESS status is timing-critical. */
	//IsAndroid();
	if((Setup[0]&0x80) == 0)//out
	{
		//if(!((Setup[3] == 0x02)&&(Setup[0] == 0x21)&&(Setup[1] == 0x09)))//audio effect out
		{
			uint32_t temp=0;
			temp = Setup[7]*256 + Setup[6];
			if(temp)
			{
				int i;
				for(i=0;i<temp/64;i++)
				{
					OTG_DeviceControlReceive(Request+i*64,64,&DataLeng,10);
				}
				if(temp%64)
				{
					OTG_DeviceControlReceive(Request+i*64,temp%64,&DataLeng,10);
				}
			}
		}
	}

	ReqType = (Setup[0]&0x60)>>5;
	//DBG("ReqType:%d\n",ReqType);
	switch(ReqType)
	{
		case 0:
			//????????????????
			//DBG("is run");
			OTG_DeviceStandardRequest();
			break;

		case 1:
			//??????????????
			OTG_DeviceClassRequest();
			break;

		case 2:
			//????????????????????
			OTG_DeviceManufacturerRequest();
			break;

		case 3:
			//????????????????????
			OTG_DeviceOtherRequest();
			break;			
	}
}

// #region agent log
void OTG_DeviceDebugDump(void)
{
	uint32_t i;
	uint32_t count = (s_dbg_setup_ok < USB_DBG_SETUP_MAX)
		? s_dbg_setup_ok : USB_DBG_SETUP_MAX;
	uint32_t usb_div_m1, usb_mux, dp_pwr, ep0_csr;
	uint32_t usb_div = 0, usb_mhz = 0, apl = 0;

	OTG_DeviceDebugHwSnapshot(&usb_div_m1, &usb_mux, &dp_pwr, &ep0_csr);
	usb_div = usb_div_m1 + 1u;
	if (usb_div)
		usb_mhz = 240u / usb_div;
	apl = (usb_mux & 0x800u) ? 1u : 0u;

	DBG("{\"sessionId\":\"5032d7\",\"runId\":\"pre-fix\",\"hypothesisId\":\"H6,H7,H9\","
	    "\"location\":\"otg_device_standard_request.c:OTG_DeviceDebugDump\","
	    "\"message\":\"usb_hw\",\"data\":{\"div\":%u,\"mhz\":%u,\"apl\":%u,"
	    "\"mux\":%u,\"dp\":%u,\"ep0csr\":%u,\"lastBus\":%u,\"lastErr\":%u,"
	    "\"rstTick\":%u,\"okTick\":%u},\"timestamp\":%u}\n",
	    (unsigned)usb_div, (unsigned)usb_mhz, (unsigned)apl,
	    (unsigned)usb_mux, (unsigned)dp_pwr, (unsigned)ep0_csr,
	    (unsigned)s_dbg_last_bus_event, (unsigned)s_dbg_last_setup_err,
	    (unsigned)s_dbg_first_reset_tick, (unsigned)s_dbg_first_ok_tick,
	    (unsigned)GetSysTick1MsCnt());

	DBG("{\"sessionId\":\"5032d7\",\"runId\":\"pre-fix\",\"hypothesisId\":\"H1,H5,H8\","
	    "\"location\":\"otg_device_standard_request.c:OTG_DeviceDebugDump\","
	    "\"message\":\"usb_ep0_summary\",\"data\":{\"reset\":%u,\"setupOk\":%u,"
	    "\"setupErr\":%u,\"short\":%u,\"configured\":%u},\"timestamp\":%u}\n",
	    (unsigned)s_dbg_bus_reset, (unsigned)s_dbg_setup_ok,
	    (unsigned)s_dbg_setup_err, (unsigned)s_dbg_setup_short,
	    (unsigned)g_usb_configured, (unsigned)GetSysTick1MsCnt());

	for (i = 0; i < count; i++) {
		const uint8_t *s = s_dbg_setup[i].setup;
		DBG("{\"sessionId\":\"5032d7\",\"runId\":\"pre-fix\",\"hypothesisId\":\"H2,H3,H4\","
		    "\"location\":\"otg_device_standard_request.c:OTG_DeviceRequestProcess\","
		    "\"message\":\"usb_setup\",\"data\":{\"index\":%u,\"bm\":%u,\"req\":%u,"
		    "\"wValue\":%u,\"wIndex\":%u,\"wLength\":%u},\"timestamp\":%u}\n",
		    (unsigned)i, (unsigned)s[0], (unsigned)s[1],
		    (unsigned)((uint16_t)s[2] | ((uint16_t)s[3] << 8)),
		    (unsigned)((uint16_t)s[4] | ((uint16_t)s[5] << 8)),
		    (unsigned)((uint16_t)s[6] | ((uint16_t)s[7] << 8)),
		    (unsigned)GetSysTick1MsCnt());
	}
}
// #endregion agent log

//*************************************************//
//*************************************************//
//*************************************************//



void hid_recive_data(void)
{
#ifdef CFG_COMMUNICATION_BY_USB
	//HIDUsb_Rx(Request,256);
#endif
}


void hid_send_data(void)
{
//	OTG_DeviceControlSend(hid_tx_buf,256,6);
}

void IsAndroid(void)
{
	/////??????????????Android???????? "A1 01 00 01 03 00 01 00"
	if( (Setup[0]==0xA1) && (Setup[1]==0x01) )//
	{
		if( (Setup[2]==0x00) && (Setup[3]==0x01) )
		{
			if( (Setup[4]==0x03) && (Setup[5]==0x00) )
			{
				if( (Setup[6]==0x01) && (Setup[7]==0x00) )
				{
					//gCtrlVars.usb_android = 1;
				}
			}
		}
	}
}
