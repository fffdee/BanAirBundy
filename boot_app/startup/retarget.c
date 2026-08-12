/*
 * retarget.c
 *
 *  Created on: Mar 8, 2017
 *      Author: peter
 */

#include <stdio.h>
#include "uarts_interface.h"
#include "type.h"
#include "remap.h"
#ifdef CFG_APP_CONFIG
#include "app_config.h"
#include "mcu_circular_buf.h"
#endif
uint8_t DebugPrintPort = UART_PORT0;

#include "uarts.h"
#include "cdc_debug.h"
//#include "rtos_api.h"
#ifdef DEBUG_LOG_INTERRUPT
	#include "nds32_intrinsic.h"
	static uint16_t IsrLogCount = 0;
	#define IS_ISR		(((uint8_t)(__nds32__mfsr(NDS32_SR_PSW) >> 16) & 0x07) < 7)
#endif

#ifdef CFG_FUNC_USBDEBUG_EN
uint8_t usb_buffer[4096];
MCU_CIRCULAR_CONTEXT usb_fifo;

void usb_hid_printf_init(void)
{
	MCUCircular_Config(&usb_fifo,usb_buffer,sizeof(usb_buffer));
}
#endif

__attribute__((used))
int putchar(int c)
{
#ifndef DEBUG_LOG_EN//注意，app_config.h中的宏定义，防止打印宏关闭之后引起异常
	return c;
#endif
	/* log -e：把 DBG/printf 镜像到 USB CDC，便于无 UART 时调试 */
	CdcDbg_MirrorChar(c);
	{
		if (c == '\n')
		{
			UART_SendByte(DebugPrintPort,'\r');
			UART_SendByte(DebugPrintPort,'\n');
		}
		else
		{
#ifdef DEBUG_LOG_INTERRUPT
			if(IsrLogCount!= 0 && c >= 'a' && c <= 'z')
			{
				c -= 0x20;//'a'-'A' = 0x20
			}
#endif
			UART_SendByte(DebugPrintPort,(uint8_t)c);
		}
	}
	return c;
}


__attribute__((used))
void nds_write(const unsigned char *buf, int size)
{
	int i;
	//usb_hid.usb_len = size;
#ifdef DEBUG_LOG_INTERRUPT
	if(IS_ISR)
	{
		IsrLogCount += size;
	}
#endif
	for (i = 0; i < size; i++)
	{
		putchar(buf[i]);
	}

#ifdef CFG_FUNC_USBDEBUG_EN
	MCUCircular_PutData(&usb_fifo,(void*)buf,size);
#endif
}

int DbgUartInit(int Which, unsigned int BaudRate, unsigned char DatumBits, unsigned char Parity, unsigned char StopBits)
{
	DebugPrintPort = Which;
	UART_Init(Which, BaudRate, DatumBits,  Parity,  StopBits);
	return 0;
}
