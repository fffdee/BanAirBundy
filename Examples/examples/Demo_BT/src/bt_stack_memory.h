/**
 **************************************************************************************
 * @file    bt_stack_memory.h
 * @brief   
 *
 * @author  kk
 * @version V1.0.0
 *
 * $Created: 2016-6-29 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __BT_STACK_MEMORY_H__
#define __BT_STACK_MEMORY_H__

#include "type.h"


/*****************************************************************
 * ram config
 *****************************************************************/
#define BT_BASE_MEM_SIZE			(8000) //common
#define BT_A2DP_MEM_SIZE			(15500)
#define BT_AVRCP_TG_MEM_SIZE		(6300)
#define BT_HFP_MEM_SIZE				(5800)

#define BT_STACK_MEM_SIZE	(BT_BASE_MEM_SIZE + BT_A2DP_MEM_SIZE + BT_AVRCP_TG_MEM_SIZE + BT_HFP_MEM_SIZE)

#endif //__BT_STACK_MEMORY_H__

