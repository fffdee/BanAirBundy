/**
 **************************************************************************************
 * @file    bt_stack_service.c
 * @brief   
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2018-2-9 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
//#include "soft_watch_dog.h"
#include <string.h>
#include "type.h"
#include "gpio.h" //for BOARD
#include "debug.h"
#include "uarts.h"
#include "uarts_interface.h"
#include "dma.h"
#include "timeout.h"
#include "irqn.h"
//#include "ble_api.h"
#include "bt_stack_service.h"
#include "bt_stack_memory.h"

#include "bt_common_api.h"
#include "bt_avrcp_api.h"
#include "bt_manager.h"
#include "bt_interface.h"
#include "bb_api.h"
#include "efuse.h"

#include "clk.h"
#include "reset.h"


#include "bt_avrcp_api.h"

static uint8_t gBtHostStackMemHeap[BT_STACK_MEM_SIZE];

#define BT_NAME		"BP15-DEMO"

BT_CONFIGURATION_PARAMS		btStackConfigParams;

extern uint32_t system_mode;

extern int8_t ME_CancelInquiry(void);
extern void A2dp_Decode(void);

/***********************************************************************************
 * 蓝牙时钟设置
 * 此函数lib中调用，客户可根据自己条件配置
 **********************************************************************************/
void BtCntClkSet(void)
{
}

/***********************************************************************************
 * 蓝牙测试盒校准频偏完成回调函数
 **********************************************************************************/
void BtFreqOffsetAdjustComplete(unsigned char offset)
{
}

/***********************************************************************************
 * 配置HOST的参数
 **********************************************************************************/
static void ConfigBtStackParams(BtStackParams *stackParams)
{
	uint32_t pCod = 0;
	if(stackParams == NULL)
		return ;

	memset(stackParams, 0 ,sizeof(BtStackParams));

	/* Set support profiles */
	stackParams->supportProfiles = 0x07;

	/* Set local device name */
	stackParams->localDevName = (uint8_t *)btStackConfigParams.bt_LocalDeviceName;

	/* Set callback function */
	stackParams->callback = BtStackCallback;
	stackParams->scoCallback = NULL;//GetScoDataFromApp;

	//simple pairing
	stackParams->btSimplePairing = btStackConfigParams.bt_simplePairingFunc;
	if((btStackConfigParams.bt_pinCodeLen)&&(btStackConfigParams.bt_pinCodeLen<17))
	{
		stackParams->btPinCodeLen = btStackConfigParams.bt_pinCodeLen;
		memcpy(stackParams->btPinCode, btStackConfigParams.bt_pinCode, stackParams->btPinCodeLen);
	}
	else
	{
		APP_DBG("ERROR:pin code len %d\n", btStackConfigParams.bt_pinCodeLen);
		stackParams->btSimplePairing = 1;//开启简易配对
	}

	//class of device
	//headset:苹果手机能显示设备的电池电量
	pCod = COD_AUDIO | COD_MAJOR_AUDIO | COD_MINOR_AUDIO_HEADSET | COD_RENDERING;
	SetBtClassOfDevice(pCod);

	/* HFP features */
	stackParams->hfpFeatures.wbsSupport = 0;//1;
	stackParams->hfpFeatures.hfpAudioDataFormat = 1;//2;
	stackParams->hfpFeatures.hfpAppCallback = BtHfpCallback;
	
	/* A2DP features */
	stackParams->a2dpFeatures.a2dpAudioDataFormat = (A2DP_AUDIO_DATA_SBC);
	stackParams->a2dpFeatures.a2dpAppCallback = BtA2dpCallback;

	/* AVRCP features */
	stackParams->avrcpFeatures.supportAdvanced = 1;
	stackParams->avrcpFeatures.supportTgSide = 0;
	stackParams->avrcpFeatures.supportSongTrackInfo = 0;
	stackParams->avrcpFeatures.supportPlayStatusInfo = 0;
	stackParams->avrcpFeatures.avrcpAppCallback = BtAvrcpCallback;

	
}

/***********************************************************************************
 * 初始化蓝牙HOST
 **********************************************************************************/
bool BtStackInit(void)
{
//	bool ret;
	int32_t retInit=0;

	BtStackParams	stackParams;

	BtManageInit();

	//BTStatckSetPageTimeOutValue(BT_PAGE_TIMEOUT); 

	//BTHostLinkNumConfig(BT_DEVICE_NUMBER, BT_SCO_NUMBER);
//	BTHostParamsConfig(&App_Bt_Host_config);

	ConfigBtStackParams(&stackParams);

	retInit = BTStackRunInit(&stackParams);
	if(retInit != 0)
	{
		APP_DBG("Bt Stack Init ErrCode [%x]\n", (int)retInit);
		return FALSE;
	}

	retInit = A2dpAppInit(&stackParams.a2dpFeatures);
    if(retInit == 0)
	{
		APP_DBG("A2dp Init ErrCode [%x]\n", (int)retInit);
		return FALSE;
	}

	retInit = AvrcpAppInit(&stackParams.avrcpFeatures);
	if(retInit == 0)
	{
		APP_DBG("Avrcp Init ErrCode [%x]\n", (int)retInit);
		return FALSE;
	}
	

	retInit = HfpAppInit(&stackParams.hfpFeatures);
	if(retInit == 0)
	{
		APP_DBG("Hfp Init ErrCode [%x]\n", (int)retInit);
		return FALSE;
	}

	return TRUE;
}

/***********************************************************************************
 * 蓝牙协议栈任务处理
 **********************************************************************************/
void BtStackServiceStart(void)
{
	BtBbParams			bbParams;
	memset((uint8_t*)0x80000000, 0, 20*1024);//clear em erea

	ClearBtManagerReg();

	SetBtStackState(BT_STACK_STATE_INITAILIZING);

	SetBluetoothMode(1<<1);
	
	//BB init
	memset(&bbParams, 0 ,sizeof(BtBbParams));
	memset(&btStackConfigParams, 0, sizeof(BT_CONFIGURATION_PARAMS));
	
	//LAP
	btManager.btDevAddr[5] = Efuse_ReadData(3);
	btManager.btDevAddr[4] = Efuse_ReadData(2);
	btManager.btDevAddr[3] = Efuse_ReadData(1);
	//UAP
	btManager.btDevAddr[2] = 0xe0;
	//NAP
	btManager.btDevAddr[1] = 0x2c;
	btManager.btDevAddr[0] = 0x1c;

	printf("bt_addr [%02x:%02x:%02x:%02x:%02x:%02x]\n",
		btManager.btDevAddr[0],
		btManager.btDevAddr[1],
		btManager.btDevAddr[2],
		btManager.btDevAddr[3],
		btManager.btDevAddr[4],
		btManager.btDevAddr[5]
		);

	strcpy((void *)btStackConfigParams.bt_LocalDeviceName, BT_NAME);

	bbParams.localDevName = (uint8_t *)btStackConfigParams.bt_LocalDeviceName;
	memcpy(bbParams.localDevAddr, btManager.btDevAddr, 6);

	bbParams.freqTrim = 0x07;

	//em config
	bbParams.em_start_addr = (256-20);

	//agc config
	bbParams.pAgcDisable = 0; //0=auto agc;	1=close agc
	bbParams.pAgcLevel = 1;

	//sniff config
	bbParams.pSniffNego = 0;//1=open;  0=close
	bbParams.pSniffDelay = 0;
	bbParams.pSniffInterval = 0x320;//500ms
	bbParams.pSniffAttempt = 0x01;
	bbParams.pSniffTimeout = 0x01;
	
	//SetRfTxPwrMaxLevel(sys_parameter.bt_TxPowerLevel, sys_parameter.bt_PagePowerLevel);

	Bluetooth_common_init(&bbParams);

	Bt_init((void*)&bbParams);

	//host memory init
	SetBtPlatformInterface(&pfiOS, NULL/*&pfiBtDdb*/);

	Name_confirm_Callback_Set(BtConnectDecide);

	//在蓝牙开启后台运行时,host的内存采用数组,避免存在申请/释放带来碎片化的风险
	BTStackMemAlloc(BT_STACK_MEM_SIZE, gBtHostStackMemHeap, 0);

	printf("BtStackServiceEntrance.\n");
	
	//BR/EDR init
	if(!BtStackInit())
	{
		printf("error init bt device\n");
		//出现初始化异常时,蓝牙协议栈任务挂起
		while(1)
		{
			;
		}
	}
	else
	{
		printf("bt device init success!\n");
	}

	SetBtStackState(BT_STACK_STATE_READY);

	BTSetAccessMode(BtAccessModeGeneralAccessible);
	
	while(1)
	{
		rw_main();
		BTStackRun();

		if(system_mode)
		{
			//
			Msbc_Decode();
		}
		else
		{
			//这个函数判断有无音频数据，如有，则解码并播放
			A2dp_Decode();
		}

	}
}

/***********************************************************************************
 * BB错误报告
 * 注:需要判断当前是否在中断中，需要调用不同的消息发送函数接口
 **********************************************************************************/
void BBMatchReport(void)
{

}

void BBErrorReport(uint8_t mode, uint32_t errorType)
{

}

/***********************************************************************************
 * BB 中断关闭
 **********************************************************************************/
void BT_IntDisable(void)
{
	NVIC_DisableIRQ(BT_IRQn);
	NVIC_DisableIRQ(BLE_IRQn);
}

/***********************************************************************************
 * BB 模块关闭
 **********************************************************************************/
void BT_ModuleClose(void)
{
//	Reset_RegisterReset(MDM_REG_SEPA);
//	Reset_FunctionReset(BTDM_FUNC_SEPA|MDM_FUNC_SEPA|RF_FUNC_SEPA);
//	Clock_Module2Disable(ALL_MODULE2_CLK_SWITCH); //close clock
}


