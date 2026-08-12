/**
 *******************************************************************************
 * @file    bt_avrcp_app.c
 * @author  KK
 * @version V1.0.0
 * @date    9-9-2023
 * @brief   Avrcp callback events and actions
 *******************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, MVSILICON SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */

#include "type.h"
#include "debug.h"
#include "bt_manager.h"
#include "bt_interface.h"
#include "string_convert.h"

#define BT_LINK_DEV_NUM  1

uint32_t AvrcpStateSuspendCount = 0;

/*****************************************************************
 * @brief  Change Player State Get
 * @param  
 * @return state: TRUE/FALSE
 * @Note   本地的播放状态判断条件
 *****************************************************************/
uint32_t ChangePlayerStateGet(void)
{
	if( AvrcpStateSuspendCount 
		&& ( (btManager.cur_index == 0 && GetA2dpState(1) == BT_A2DP_STATE_STREAMING) || (btManager.cur_index == 1 && GetA2dpState(0) == BT_A2DP_STATE_STREAMING) ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*****************************************************************
 * @brief  bt avrcp callback function
 * @param  event: BT_AVRCP_CALLBACK_EVENT
 * @param  param: BT_AVRCP_CALLBACK_PARAMS
 * @return 
 * @Note   avrcp协议回调事件/参数
 *****************************************************************/
void BtAvrcpCallback(BT_AVRCP_CALLBACK_EVENT event, BT_AVRCP_CALLBACK_PARAMS * param)
{
	switch(event)
	{
		case BT_STACK_EVENT_AVRCP_CONNECTED:
		{
			printf("avrcp connect\n");
			break;
		}

		case BT_STACK_EVENT_AVRCP_DISCONNECTED:
		{
			printf("avrcp disconnect\n");
			break;
		}
		case BT_STACK_EVENT_AVRCP_ADV_PLAY_STATUS_CHANGED:
		{
			printf("avrcp status: %d\n", param->params.avrcpAdv.avrcpAdvMediaStatus);
			break;
		}
		case BT_STACK_EVENT_AVRCP_ADV_PLAY_STATUS:
			break;

		case BT_STACK_EVENT_AVRCP_ADV_TRACK_INFO:
			break;

		case BT_STACK_EVENT_AVRCP_ADV_ADDRESSED_PLAYERS:
			break;

		case BT_STACK_EVENT_AVRCP_ADV_CAPABILITY_COMPANY_ID:
			break;
			
		case BT_STACK_EVENT_AVRCP_ADV_CAPABILITY_EVENTS_SUPPORTED:
			break;

///////////////////////////////////////////////////////////////////////////////////////////
		default:
			break;
	}
}


/*****************************************************************
 * @brief  change player
 * @param  
 * @return 
 * @Note   双手机连接切换播放流程
 *****************************************************************/
void ChangePlayer(void)
{
	/*if((PreChangeFlag == TRUE) && IsTimeOut(&tChangePlayer))
	{
		BtCurIndex_Set(btManager.back_index);
		BT_DBG("channel[%d]:start playing\n", btManager.cur_index);
		//if(RefreshSbcDecoder)
		{
			//RefreshSbcDecoder();
			BtMidMessageSend(MSG_BT_MID_PLAY_STATE_CHANGE, AVRCP_ADV_MEDIA_PLAYING);
		}
		PreChangeFlag = FALSE;
	}*/
#if (BT_LINK_DEV_NUM == 2)
	if(AvrcpStateSuspendCount)
	{
		AvrcpStateSuspendCount++;
		if(AvrcpStateSuspendCount >= 1000)
		{
			uint8_t switch_index = (btManager.cur_index == 0) ? 1 : 0;

			if(GetA2dpState(switch_index) == BT_A2DP_STATE_STREAMING)
			{
#ifdef LAST_PLAY_PRIORITY
				APP_DBG("ChangePlayer btManager.cur_index Pause %d %d\n",btManager.btLinked_env[btManager.cur_index].avrcp_index,GetA2dpState(btManager.cur_index));
				AvrcpCtrlPause(btManager.btLinked_env[btManager.cur_index].avrcp_index);
				SetA2dpState(btManager.cur_index,BT_A2DP_STATE_CONNECTED);
#endif
				BtCurIndex_Set(switch_index);
				a2dp_sbc_decoer_init();
			}
			AvrcpStateSuspendCount = 0;
		}
	}
#endif

}

/*****************************************************************
 * @brief  avrcp link state set
 * @param  index: 0 or 1
 * @param  state: BT_AVRCP_STATE
 * @Note   
 *****************************************************************/
void SetAvrcpState(uint8_t index, BT_AVRCP_STATE state)
{
	btManager.btLinked_env[index].avrcpState = state;
}

/*****************************************************************
 * @brief  avrcp link state get
 * @param  index: 0 or 1
 * @return state: BT_AVRCP_STATE
 * @Note   
 *****************************************************************/
BT_AVRCP_STATE GetAvrcpState(uint8_t index)
{
	return btManager.btLinked_env[index].avrcpState;
}

/*****************************************************************
 * @brief  avrcp current play state set
 * @param  index: 0 or 1
 * @param  state: uint8_t
 * @Note   
 *****************************************************************/
void SetAvrcpCurPlayState(uint8_t index, uint8_t state)
{
	btManager.btLinked_env[index].avrcpCurPlayState = state;
}

/*****************************************************************
 * @brief  avrcp current play state get
 * @param  index: 0 or 1
 * @return state: uint8_t
 * @Note   
 *****************************************************************/
uint8_t GetAvrcpCurPlayState(uint8_t index)
{
	return btManager.btLinked_env[index].avrcpCurPlayState;
}

/*****************************************************************
 * @brief  avrcp link state is connected?
 * @param  index: 0 or 1
 * @return state: 1=connected
 * @Note   
 *****************************************************************/
bool IsAvrcpConnected(uint8_t index)
{
	return (GetAvrcpState(index) == BT_AVRCP_STATE_CONNECTED);
}

/*****************************************************************
 * @brief  avrcp connect
 * @param  index: 0 or 1
 * @param  addr: remote address
 * @Note   avrcp协议连接接口
 *****************************************************************/
void BtAvrcpConnect(uint8_t index, uint8_t *addr)
{
#if (BT_LINK_DEV_NUM == 2)
	uint8_t avrcp_index = GetBtManagerAvrcpIndex(index);

	if( avrcp_index < BT_LINK_DEV_NUM
		&&(btManager.btLinked_env[avrcp_index].avrcpState > BT_AVRCP_STATE_NONE))
	{
		APP_DBG("BtAvrcpConnect:avrcpState index[%d] is %d\n",avrcp_index, btManager.btLinked_env[avrcp_index].avrcpState);
		return;
	}
#endif
	APP_DBG("AvrcpConnect index = %d,addr:%x:%x:%x:%x:%x:%x\n", index, addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
	AvrcpConnect(index, addr);
}

