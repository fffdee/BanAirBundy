////////////////////////////////////////////////////////////////////////////////
//                   Mountain View Silicon Tech. Inc.
//		Copyright 2011, Mountain View Silicon Tech. Inc., ShangHai, China
//                   All rights reserved.
//
//		Filename	:debug.h
//
//		Description	:
//					Define debug ordinary print & debug routine
//
//		Changelog	:
///////////////////////////////////////////////////////////////////////////////

#ifndef __DEBUG_H__
#define __DEBUG_H__

/**
 * @addtogroup mv_utils
 * @{
 * @defgroup debug debug.h
 * @{
 */
 
#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include <stdio.h>
#include "type.h"
#include "gpio.h"

#define DEBUG_GPIO_UARTPORT_UART0										0
#define DEBUG_GPIO_UARTPORT_UART1										1

#define DEBUG_GPIO_PORT_A												0
#define DEBUG_GPIO_PORT_B												1

#define SET_DEBUG_GPIO_VAL(uart_port,gpio_mode,gpio_port,gpio_index)	((uart_port<<24)|(gpio_mode<<16)|(gpio_port<<8)|(gpio_index))
#define GET_DEBUG_GPIO_UARTPORT(val)									((val>>24)&0x01)
#define GET_DEBUG_GPIO_MODE(val)										((val>>16)&0x0f)
#define GET_DEBUG_GPIO_PORT(val)										((val>>8)&0x01)
#define GET_DEBUG_GPIO_INDEX(val)										((val)&0x1f)
#define GET_DEBUG_GPIO_PIN(val)											(1<<((val)&0x1f))

enum
{
	DEBUG_BAUDRATE_9600 	= 9600,
	DEBUG_BAUDRATE_115200 	= 115200,
	DEBUG_BAUDRATE_256000	= 256000,
	DEBUG_BAUDRATE_1000000	= 1000000,
	DEBUG_BAUDRATE_2000000	= 2000000,
	DEBUG_BAUDRATE_3000000 	= 3000000,
};

enum
{
	DEBUG_TX_A0  = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART0, 0x05, DEBUG_GPIO_PORT_A, 0),
	DEBUG_TX_A1  = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART0, 0x05, DEBUG_GPIO_PORT_A, 1),
	DEBUG_TX_A6  = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART0, 0x07, DEBUG_GPIO_PORT_A, 6),
	DEBUG_TX_A10 = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x05, DEBUG_GPIO_PORT_A, 10),
	DEBUG_TX_A18 = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x04, DEBUG_GPIO_PORT_A, 18),
	DEBUG_TX_A19 = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x03, DEBUG_GPIO_PORT_A, 19),
	DEBUG_TX_A20 = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x0d, DEBUG_GPIO_PORT_A, 20),
	DEBUG_TX_A21 = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x0c, DEBUG_GPIO_PORT_A, 21),
	DEBUG_TX_A22 = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x08, DEBUG_GPIO_PORT_A, 22),
	DEBUG_TX_A23 = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x07, DEBUG_GPIO_PORT_A, 23),
	DEBUG_TX_A24 = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x07, DEBUG_GPIO_PORT_A, 24),
	DEBUG_TX_A28 = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x09, DEBUG_GPIO_PORT_A, 28),
	DEBUG_TX_A29 = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x08, DEBUG_GPIO_PORT_A, 29),
	DEBUG_TX_A30 = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x08, DEBUG_GPIO_PORT_A, 30),
	DEBUG_TX_A31 = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x08, DEBUG_GPIO_PORT_A, 31),
	DEBUG_TX_B5  = SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x04, DEBUG_GPIO_PORT_B, 5),
};

//用define定义UART RX引脚，方便用宏裁剪代码
#define	DEBUG_RX_A0		SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART0, 0x01, DEBUG_GPIO_PORT_A, 0)
#define	DEBUG_RX_A1  	SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART0, 0x02, DEBUG_GPIO_PORT_A, 1)
#define	DEBUG_RX_A5  	SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART0, 0x02, DEBUG_GPIO_PORT_A, 5)
#define	DEBUG_RX_A9  	SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x01, DEBUG_GPIO_PORT_A, 9)
#define	DEBUG_RX_A18 	SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x01, DEBUG_GPIO_PORT_A, 18)
#define	DEBUG_RX_A19 	SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x01, DEBUG_GPIO_PORT_A, 19)
#define	DEBUG_RX_B4  	SET_DEBUG_GPIO_VAL(DEBUG_GPIO_UARTPORT_UART1, 0x02, DEBUG_GPIO_PORT_B, 4)

/************************** DEBUG**********************************
*下面为打印调试信息的接口,共有两个:
*1.打印前缀接口 : 
*  1)如果需要在打印时输出模块前缀 ,请调用 APP_DBG(),
*  2)需要关闭某模块的调试信息,可以用注释屏蔽 XXX_MODULE_DEBUG的宏
*	此模块宏,是根据如下规则 来划分的 : 
*	共划分为如下9个模块,
*	其中7个模块在Apps目录:分为media play,bt,hdmi_in,Main_task,usb_audio_mode,waiting_mode模块
*	另外两个模块是:Device模块、Services模块
*	剩下统一放在：DEB模块
*
*2.不打印前缀接口 : 调用 DBG()
*************************************************************/
#define	APP_DBG(format, ...)	printf(format, ##__VA_ARGS__)
#define	DBG(format, ...)		printf(format, ##__VA_ARGS__)
#define	OTG_DBG(format, ...)		//printf(format, ##__VA_ARGS__)
#define	BT_DBG(format, ...)		printf(format, ##__VA_ARGS__)


int DbgUartInit(int Which, unsigned int BaudRate, unsigned char DatumBits, unsigned char Parity, unsigned char StopBits);



/************************** TOGGLE DEBUG**********************************
 *
 * 为了方便调试时序，debug文件提供IO toggle功能
 * 1. LED_IO_TOGGLE 端口和序号配置列表是IO，实际api按0~n 需要使用，
 * 2、OS_INT_TOGGLE Os任务和中断时间片 观测，以task名字表为序号，led0~x，中断排在之后，需要先开启LED_IO_TOGGLE
 *
 *************************************************************/

/**OS任务切换和中断 时IO toggle，高电平表征执行时间片**/
//#define OS_INT_TOGGLE

/**需要toggle的task Name,可用部分,建议优先级高的在前；顺序 对应LED 0、LED 1....**/
#define DBG_TASK_LIST	{{"AudioCore"}, {"SysMode"}, {"MainApp"}, {"BtStack"}, {"IDLE"}}
//uint8_t dbgtasklist[][configMAX_TASK_NAME_LEN];
void DbgTaskTGL_set();
void DbgTaskTGL_clr();


/*********************************************
 * 中断toggle入口，需要修缮crt0.S的OS_Trap_Int_Comm 代码如下文

 	 原先语句代码:
 	jral $r1

 	修改为:
 	pushm $r0,$r1
 	jal   OS_dbg_int_in
 	popm $r0,$r1
 	push $r0
	jral $r1
 	pop  $r0
 	jal   OS_dbg_int_out
 ********************************************/
/**中断toggle的IO(LED_PORT_LIST) 排在 OS 序号后面,单列DBG_INT_ID, 其他非OS中断用一个IO**/
#define DBG_INT_ID				18 //Int18为BT INT0为OS 其他参见crt0.S

void OS_dbg_int_in(uint32_t int_num);
void OS_dbg_int_out(uint32_t int_num);



//#define LED_IO_TOGGLE //使用IO toggle调试  led或逻分观测 配合 LedPortInit()


/**测试IO,常用于程序执行时序可视化，标准SDK不使用**/
/**输出高低电平，上升沿，下降沿，翻转等，需配置LedOn有效电平。**/
/**调用LedPortInit初始化端口配置。**/
#define LED_ON_LEVEL	1//灯亮电平 设定。
/**根据需要启动对应端口和序号,led 0~n-1，注意端口复用规避**/
#define LED_PORT_LIST	{{'A', 0}, {'A', 1}, {'A', 2}, {'A', 3}, {'A', 4}, {'A', 5}, {'A', 6}, {'A', 7}, {'A', 8}, {'A', 15}, {'A', 16}, {'A', 17}, {'A', 20}, {'A', 21}, {'A', 22}, {'A', 23}, {'A', 24}, {'A', 28}, {'A', 29}}



#define PORT_IN_REG(X)			(X=='A'? GPIO_A_IN : GPIO_B_IN)
#define PORT_OUT_REG(X)			(X=='A'? GPIO_A_OUT : GPIO_B_OUT)
#define PORT_SET_REG(X)			(X=='A'? GPIO_A_SET : GPIO_B_SET)
#define PORT_CLR_REG(X)			(X=='A'? GPIO_A_CLR : GPIO_B_CLR)
#define PORT_TGL_REG(X)			(X=='A'? GPIO_A_TGL : GPIO_B_TGL)
#define PORT_IE_REG(X)			(X=='A'? GPIO_A_IE : GPIO_B_IE)
#define PORT_OE_REG(X)			(X=='A'? GPIO_A_OE : GPIO_B_OE)
#define PORT_DS_REG(X)			(X=='A'? GPIO_A_DS : GPIO_B_DS)
#define PORT_PU_REG(X)			(X=='A'? GPIO_A_PU : GPIO_B_PU)
#define PORT_PD_REG(X)			(X=='A'? GPIO_A_PD : GPIO_B_PD)
#define PORT_ANA_REG(X)			(X=='A'? GPIO_A_ANA_EN : GPIO_B_ANA_EN)
#define PORT_PULLDOWN_REG(X)	(X=='A'? GPIO_A_PULLDOWN : GPIO_B_PULLDOWN)
#define PORT_CORE_REG(X)		(X=='A'? GPIO_A_CORE_OUT_MASK : GPIO_B_CORE_OUT_MASK)
#define PORT_DMA_REG(X)			(X=='A'? GPIO_A_DMA_OUT_MASK : GPIO_B_DMA_OUT_MASK)
#define SET_MOD_REG(X, I, Mode)			((X)=='A'? (GPIO_PortAModeSet((I), (Mode))) : GPIO_PortBModeSet((I), (Mode)))

void LedPortInit(void);
void LedOn(uint8_t Index);//1~4 是否生效取决于LEDx_PORT是否定义
void LedOff(uint8_t Index);
void LedToggle(uint8_t Index);
void LedPortRise(uint8_t Index);
void LedPortDown(uint8_t Index);
bool LedPortGet(uint8_t Index);//TRUE:高电平，FALSE:低电平，index无效时 默认FALSE

#ifdef __cplusplus
}
#endif//__cplusplus

/**
 * @}
 * @}
 */
 
#endif //__DBG_H__ 

