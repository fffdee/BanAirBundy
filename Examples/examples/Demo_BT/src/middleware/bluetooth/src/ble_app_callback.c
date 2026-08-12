////////////////////////////////////////////////
//
//
#include "debug.h"

#include "ble_api.h"

#ifdef CFG_APP_CONFIG
#include "app_config.h"
#include "bt_play_mode.h"
#endif
#include "rtos_api.h"
#include "bt_config.h"
#if (BLE_SUPPORT == ENABLE)

uint8_t BleConnectFlag = 0;

static void btAddrToString(unsigned char addr[6], char *str)
{
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}

uint8_t GetBLEConnState(void)
{
    return BleConnectFlag;
}

void LeAppRegCB(LeParamCB LeCBFunc)
{
    LeCallBackAPP = LeCBFunc;
}
uint8_t test_buff[]={0x66};
void AppEventCallBack(LE_CB_EVENT event, LE_CB_PARAMS *param)
{
	char addr[12];
    switch (event)
    {
    case LE_INIT_STATUS:
    	if(param->le_init_status == 0x00)
    	{
		  BLE_INFO("BLE INIT OK!!!\n");
    	}
    	else{
    		BLE_INFO("\n----> LE INIT FAILED <-----\n status: 0x%02x\n",param->le_init_status);
    	}
        break;
    case LE_CONNECTED:
    	printf("\n***********LE_CONNECTED************\n");
        BLE_INFO("connect handle: 0x%04x\n", param->con_params->conhdl);
        btAddrToString(&param->con_params->peer_addr.addr[0],addr);
        BLE_INFO("Address: %s",addr);
        break;
    case LE_DISCONNECT:
    	printf("\n***********LE_DISCONNECT************\n");
        BLE_INFO("disconnect reson: 0x%02x\ndisconnect handle: 0x%04x\nindex: 0x%04x\n",
               param->dis_params->reason, param->dis_params->conhdl, param->dis_params->conidx);
        break;
    case LE_CONNECT_PARAMS_UPDATE:
    	printf("\n*****LE_CONNECT_PARAMS_UPDATE*****\n");
        BLE_INFO("con_interval: %d\ncon_latency: %d\nSupervision timeout: %d\n",
               param->con_update_param->con_interval, param->con_update_param->con_latency, param->con_update_param->sup_to);
        break;
    case LE_MTU_EXCHANGE_RESULT:{
    	printf("\n*****LE_MTU_EXCHANGE_RESULT*****\n");
    	le_connection_param_t par;
    	par.intv_max = 25;
    	par.intv_min = 25;
    	par.latency = 0;
    	par.time_out = 2000;
//        app_update_param(&par);
    	BLE_INFO("MTU: %d\n",param->mtu_size);
    }
    	break;
    case LE_RCV_DATA_EVENT:
    {
        uint8_t i;
        printf("\n*****LE_RCV_DATA_EVENT*****\n");
        BLE_INFO("connect handle: 0x%04x,att_handle: 0x%04x\n", param->rcv_data.conhdl, param->rcv_data.handle);
        BLE_INFO("RCV DATA: ");
        for (i = 0; i < param->rcv_data.len; i++)
        {
            printf("0x%02x,", param->rcv_data.data[i]);
        }
        printf("\n");
        break;
    }
    case LE_APP_READ_DATA_EVENT:
    {
    	printf("\n*****LE_APP_READ_DATA_EVENT*****\n");
        switch (param->att_handle)
        {
        case 0:
            break;

        default:
         	BLE_INFO("\n*****test_buff*****\n");
         	BLE_INFO("param->att_handle: 0x%04x\n",param->att_handle);
         	ble_read_data.len =sizeof(test_buff);
         	ble_read_data.data = test_buff;
            break;
        }

        break;
    }
    default:
        break;
    }
}

#endif

