/**
 **************************************************************************************
 * @file    uart_example.c
 * @brief   uart
 * 
 * @author  Ben
 * @version V1.0.1
 * 
 * $Id$
 * $Created: 2023-12-07
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <nds32_intrinsic.h>
#include "gpio.h"
#include "uarts.h"
#include "uarts_interface.h"
#include "type.h"
#include "debug.h"
#include "spi_flash.h"
#include "timeout.h"
#include "clk.h"
#include "pwm.h"
#include "delay.h"
#include "rtc.h"
#include "spis.h"
#include "watchdog.h"
#include "irqn.h"
#include "spi_flash.h"
#include "remap.h"
#include "bmd.h"
#include "chip_info.h"
#include "sw_uart.h"
#include "core_d1088.h"
#include "dma.h"

#define NEW_EXAMPLE
extern void SysTickInit(void);

#define DMA_TST_BUF_LEN 10
static uint8_t DmaTxFifo[512];
static uint8_t DmaTxBuf[512];
static uint8_t DmaTxBuf1[512];
static uint8_t DmaRxBuf[1024];
static uint8_t DmaTempBuf[512];
static uint8_t DmaTempBuf1[512];
uint8_t BuartRGBuf[1024];
static uint8_t crc_check_buf[DMA_TST_BUF_LEN+1];
BUFFER_INFO g_dma_ring_buf = {0};
static uint16_t dLenInDma = 0;

static uint8_t DmaChannelMap[6] = {
		PERIPHERAL_ID_UART0_RX,
		PERIPHERAL_ID_UART1_RX,
		PERIPHERAL_ID_UART1_TX,
		PERIPHERAL_ID_UART0_TX,
		255,
		255,
};


static uint8_t rxdata;
int flag=0;
__attribute__((section(".driver.isr")))
void UART0_Interrupt(void)
{
    if(UARTS_IOCTL(0,UART_IOCTL_RXSTAT_GET, 1) & 0x01)
    {
    	UARTS_RecvByte(0,&rxdata);
        UARTS_IOCTL(0,UART_IOCTL_RXINT_CLR, 1);
        flag = rxdata;

        //演示：接收到的发送
        DBG("\n[Recieved data]:\n");
        UARTS_SendByte(0,rxdata); DBG("%c",rxdata);

		//实际使用中，要在中断处理函数外面处理数据，否则可能影响效率。
		//（注意:RX FIFO为4Bytes）这种模式下，由于Cache的存在，第一次执行中断入口函数中的代码效率会比后面低
	}

	if( UARTS_IOCTL(0,UART_IOCTL_RX_ERR_INT_GET, 0)
		&& (UARTS_IOCTL(0,UART_IOCTL_RXSTAT_GET, 0) & 0x1C))//ERROR INT
	{
		UARTS_IOCTL(0,UART_IOCTL_RX_ERR_CLR, 1);
//		UARTS_IOCTL(0,UART_IOCTL_TXFIFO_CLR, 1);
		UARTS_IOCTL(0,UART_IOCTL_RXFIFO_CLR, 1);
		DBG("\nUART_IOCTL_RX_ERR_INT_GET\n");
    }

//    if(UARTS_IOCTL(0,UART_IOCTL_TXSTAT_GET, 0)) { //TX Finished INT
//    	UARTS_IOCTL(0,UART_IOCTL_TXINT_CLR, 1);
//    	//DBG("\nUART_IOCTL_TXSTAT_GET\n");
//    }

    if(UARTS_IOCTL(0,UART_IOCTL_RXSTAT_GET, 0) & 0x02) { //OVERTIME INT
    	UARTS_IOCTL(0,UART_IOCTL_OVERTIME_CLR, 1);
    	DBG("\nUART_IOCTL_RXSTAT_GET\n");
    }
}
static void DmaInterruptUart1Tx(void)
{

    UARTS_DMA_TxIntFlgClr(UART_PORT1, DMA_DONE_INT);
    UARTS_DMA_TxIntFlgClr(UART_PORT1, DMA_THRESHOLD_INT);
    UARTS_DMA_TxIntFlgClr(UART_PORT1, DMA_ERROR_INT);
}
static void DmaInterruptUart0Tx(void)
{
	static int Flag = 0;
	if(DMA_InterruptFlagGet(PERIPHERAL_ID_UART0_TX, DMA_THRESHOLD_INT))
	{
		if(Flag==0)
		{
			DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, DmaTempBuf, sizeof(DmaTempBuf)/2);
			Flag = 1;
		}else
		{
			DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, DmaTempBuf1, sizeof(DmaTempBuf1)/2);
			Flag = 0;
		}

		UARTS_DMA_TxIntFlgClr(UART_PORT0, DMA_THRESHOLD_INT);
		//DBG("\nUART1 TX DMA_THRESHOLD_INT\n");
	}
	if(DMA_InterruptFlagGet(PERIPHERAL_ID_UART0_TX, DMA_DONE_INT))
	{
		UARTS_DMA_TxIntFlgClr(UART_PORT0, DMA_DONE_INT);
		DBG("\nUART1 TX DMA_DONE_INT\n");
	}
	if(DMA_InterruptFlagGet(PERIPHERAL_ID_UART0_TX, DMA_ERROR_INT))
	{
		UARTS_DMA_TxIntFlgClr(UART_PORT0, DMA_ERROR_INT);
		DBG("\nUART1 TX DMA_ERROR_INT\n");
	}
}

static void DmaBlockInterruptUart0Tx(void)
{

	if(DMA_InterruptFlagGet(PERIPHERAL_ID_UART0_TX, DMA_DONE_INT))
	{
		//block send step 6:处理传输完成中断函数
		UARTS_DMA_TxIntFlgClr(UART_PORT0, DMA_DONE_INT);//清DMA_DONE_INT中断
		DMA_ChannelDisable(PERIPHERAL_ID_UART0_TX);//关闭DmaChannel，用的时候再打开
		UARTS_IOCTL(UART_PORT0,UART_IOCTL_DMA_TX_EN, 0);//关闭DMA_TX,用的时候再打开
		//DBG("\nUART1 TX DMA_DONE_INT\n");
	}
	if(DMA_InterruptFlagGet(PERIPHERAL_ID_UART0_TX, DMA_THRESHOLD_INT))
	{
		UARTS_DMA_TxIntFlgClr(UART_PORT0, DMA_THRESHOLD_INT);
//		DMA_ChannelDisable(PERIPHERAL_ID_UART1_TX);//关闭DmaChannel，用的时候再打开
//		UARTS_IOCTL(UART_PORT1,UART_IOCTL_DMA_TX_EN, 0);//关闭DMA_TX,用的时候再打开
		DBG("\nUART1 TX DMA_THRESHOLD_INT\n");
	}
	if(DMA_InterruptFlagGet(PERIPHERAL_ID_UART0_TX, DMA_ERROR_INT))
	{
		UARTS_DMA_TxIntFlgClr(UART_PORT0, DMA_ERROR_INT);
		DBG("\nUART1 TX DMA_ERROR_INT\n");
	}
}
static void DmaInterruptUart1Rx(void)
{
    int32_t Echo, _result;
    Echo = UARTS_DMA_RecvDataStart(UART_PORT0, DmaTempBuf, DMA_TST_BUF_LEN);
    Buf_GetRoomLeft(&g_dma_ring_buf, _result);
    if(_result > Echo) {
        Buf_Push_Multi(&g_dma_ring_buf, DmaTempBuf, Echo, 0);
		dLenInDma += Echo;
    } else {
        DBG("uart0 ring buffer overflow\n");
    	;
    }
    UARTS_DMA_RxIntFlgClr(UART_PORT0, DMA_DONE_INT);
    UARTS_DMA_RxIntFlgClr(UART_PORT0, DMA_THRESHOLD_INT);
    UARTS_DMA_RxIntFlgClr(UART_PORT0, DMA_ERROR_INT);
}

static void DmaInterruptUart0Rx(void)
{
    int32_t Echo, _result;
    Echo = UARTS_DMA_RecvDataStart(UART_PORT0, DmaTempBuf, DMA_TST_BUF_LEN);
    Buf_GetRoomLeft(&g_dma_ring_buf, _result);
    if(_result > Echo) {
        Buf_Push_Multi(&g_dma_ring_buf, DmaTempBuf, Echo, 0);
    } else {
        DBG("uart1 ring buffer overflow\n");
    }

	if(DMA_InterruptFlagGet(PERIPHERAL_ID_UART0_RX, DMA_DONE_INT))
	{
		UARTS_DMA_RxIntFlgClr(UART_PORT0, DMA_DONE_INT);
		DBG("UART1 RX DMA_DONE_INT\n");
	}
	if(DMA_InterruptFlagGet(PERIPHERAL_ID_UART0_RX, DMA_THRESHOLD_INT))
	{
		UARTS_DMA_RxIntFlgClr(UART_PORT0, DMA_THRESHOLD_INT);//及时清水位中断
		//DBG("\n\t\t\tUART1 RX DMA_THRESHOLD_INT\n");
	}
	if(DMA_InterruptFlagGet(PERIPHERAL_ID_UART0_RX, DMA_ERROR_INT))
	{
		UARTS_DMA_RxIntFlgClr(UART_PORT0, DMA_ERROR_INT);
		DBG("UART1 RX DMA_ERROR_INT\n");
	}
}

INT_FUNC DmaIntTxFunArry[2] = {
    DmaInterruptUart0Tx,
    DmaInterruptUart1Tx,
};

INT_FUNC DmaIntRxFunArry[2] = {
    DmaInterruptUart0Rx,
    DmaInterruptUart1Rx,
};
void TGPIO_UartRxIoConfig(int8_t Which, uint8_t ModeSel)//B5
{
	DBG("TGPIO_UartRxIoConfig---Which:%d , ModeSel:%d\n",Which,ModeSel);
    switch(Which)
    {
        case	0:
        		if(ModeSel == 0)
        	    {
        	         GPIO_PortAModeSet(GPIOA0, 0x1);//A0 UART RX
        	    }
        	    else if(ModeSel == 1)
        	    {
        	         GPIO_PortAModeSet(GPIOA5, 0x2); //A5 UART RX
        	    }
        	    else if(ModeSel == 2)
        	    {
        	         GPIO_PortAModeSet(GPIOA1, 0x2); //A1 UART RX
        	    }
                break;

        case	1:
    		if(ModeSel == 0)
    	    {
    	         GPIO_PortBModeSet(GPIOB4, 0x2); //B4 UART RX
    	         //SREG_GPIO_MUXSEL4.B4_SEL = 0x2;
    	    }
    	    else if(ModeSel == 1)
    	    {
    	    	 GPIO_PortAModeSet(GPIOA9, 0x1);//A9 UART RX
    	    }
    	    else if(ModeSel == 2)
    	    {
    	    	 GPIO_PortAModeSet(GPIOA18, 0x1); //A18 UART RX
    	    }
    	    else if(ModeSel == 3)
    	    {
    	    	GPIO_PortAModeSet(GPIOA19, 0x1); //A19 UART RX
    	    }

                break;
        default:
                break;
    }
}



void TGPIO_UartTxIoConfig(int8_t Which, uint8_t ModeSel)//B5
{
	DBG("TGPIO_UartTxIoConfig---Which:%d , ModeSel:%d\n",Which,ModeSel);
    switch(Which)
    {
        case	0:
        		if(ModeSel == 0)
        	    {
        			GPIO_PortAModeSet(GPIOA1, 0x5);//A1 UART TX
        	    }
        	    else if(ModeSel == 1)
        	    {
        	        GPIO_PortAModeSet(GPIOA6, 0x7); //A6 UART TX
        	    }
        	    else if(ModeSel == 2)
        	    {
        	        GPIO_PortAModeSet(GPIOA0, 0x5); //A0 UART TX
        	    }
                break;

        case	1:
                if(ModeSel == 0)
                {
                	GPIO_PortBModeSet(GPIOB5, 0x4); //B5 UART TX
                	// SREG_GPIO_MUXSEL4.B5_SEL = 0x4;
                }
                else if(ModeSel == 1)
                {
                	GPIO_PortAModeSet(GPIOA10, 0x5); //A10 UART TX
                }
                else if(ModeSel == 2)
                {
                	GPIO_PortAModeSet(GPIOA19, 0x3); //A19 UART TX
                }
                else if(ModeSel == 3)
                {
                	GPIO_PortAModeSet(GPIOA18, 0x4); //A18 UART TX
                }
                else if(ModeSel == 4)
                {
                	GPIO_PortAModeSet(GPIOA20, 0xd); //A20 UART TX
                }
                else if(ModeSel == 5)
                {
                	GPIO_PortAModeSet(GPIOA21, 0xc); //A21 UART TX
                }
                else if(ModeSel == 6)
                {
                	GPIO_PortAModeSet(GPIOA22, 0x8); //A22 UART TX
                }
                else if(ModeSel == 7)
                {
                	GPIO_PortAModeSet(GPIOA23, 0x7); //A23 UART TX
                }
                else if(ModeSel == 8)
                {
                	GPIO_PortAModeSet(GPIOA24, 0x7); //A24 UART TX
                	//SREG_GPIO_MUXSEL2.A24_SEL = 0x7;
                }
                else if(ModeSel == 9)
                {
                	GPIO_PortAModeSet(GPIOA28, 0x9); //A28 UART TX
                }
                else if(ModeSel == 10)
                {
                	GPIO_PortAModeSet(GPIOA29, 0x8); //A29 UART TX
                }
                else if(ModeSel == 11)
                {
                	GPIO_PortAModeSet(GPIOA30, 0x8); //A30 UART TX
                }
                else if(ModeSel == 12)
                {
                	GPIO_PortAModeSet(GPIOA31, 0x8); //A31 UART TX
                }
                break;
        default:
                break;
    }
}

static int32_t WaitDatum1Ever(void)//从串口等待接收一个字节
{
	uint8_t Data;
	DBG("*******************Please input a num (0~9)*********************\n");
	do{
		if(0 < UARTS_Recv(1,&Data, 1,10))
        {
			break;
		}
	}while(1);
	return Data-0x30;
}

void clean_ring_buffer(BUFFER_INFO *buf)
{
    Buf_Flush(buf);
    memset(buf->CharBuffer, 0x00, buf->Length);

}

void Example_UART_MCU_NonInt(void)
{
	uint8_t Buf[100];
	uint8_t len = 0;
	uint32_t cnt = 0;
	//说明：Example 默认使用UART0的PORT 0口作为示例;
	//step 0:确认UART的时钟是否开启
	//step 1:配置GPIO复用
	TGPIO_UartRxIoConfig(UART_PORT0,1);//RX A5
	TGPIO_UartTxIoConfig(UART_PORT0,1);//TX A6

	//step 2:配置UART参数
	UARTS_Init(0,2000000, 8, 0, 1);

	//step 3:定期接收UART数据
	DBG("\n***** you will receive what you have sent *****\n");
	while(1)
	{
		if((UARTS_IOCTL(0,UART_IOCTL_RXSTAT_GET, 1) & 0x01)&&(cnt%10==0))//接收的频次变稀，则可能会错过数据
		{
		     len = UARTS_Recv(0,Buf,10,100);//TimeOut增大，一次最长接收的长度则变长
		     UARTS_IOCTL(0, UART_IOCTL_RXINT_CLR, 1);
		     if(len > 0)//如果接收到数据
		     {
		    	 UARTS_Send(0,Buf,len,20);//把接收到的数据打印出来
		     }
		}
		cnt++;
	}
}

void Example_UART_MCU_Int(void)
{
	//说明：Example 默认使用UART0的PORT 0口作为示例;

	//step 0:确认UART的时钟是否开启
	//step 1:配置GPIO复用
	TGPIO_UartRxIoConfig(UART_PORT0,1);//RX A5
	TGPIO_UartTxIoConfig(UART_PORT0,1);//TX A6

	//step 2:配置UART参数
	UARTS_Init(0,2000000, 8, 0, 1);//UART0_Init(2000000, 8, 0, 1);

	//step 3:配置中断
	//  注意中断入口函数是否设置,如果没有设置，使用默认入口UART0_Interrupt
	NVIC_EnableIRQ(UART0_IRQn);
	UARTS_IOCTL(0,UART_IOCTL_RXINT_SET, 1);
	UARTS_IOCTL(0,UART_IOCTL_RXINT_CLR, 1);
	UARTS_IOCTL(0,UART_IOCTL_RXFIFO_CLR, 1);

	DBG("\n***** you will receive what you have sent *****\n");
	while(1);;;
}
void Example_UART_DMA_Block_Send(void)
{
	uint8_t buf[] = "Buf0 data has been send\n";
	uint8_t buf1[] = "Buf1 data has been send\n";
	memset(DmaTxBuf,0x38,512);
	memset(DmaTxBuf1,0x39,512);
	DBG("\n***** Select 0 1 2 3  *****\n");
	DBG("***** to choose different sendbuf *****\n");

	//说明：Example 默认使用UART0的PORT 0口作为示例;

	//block send step 0:确认UART的时钟是否开启,见mian()
	//block send step 1:配置GPIO复用
	TGPIO_UartRxIoConfig(UART_PORT0,1);//RX A5
	TGPIO_UartTxIoConfig(UART_PORT0,1);//TX A6

	//block send step 2:配置UART参数
	UARTS_Init(0,2000000,8,0,1);

	//block send step 3:配置DMA
	DMA_ChannelAllocTableSet(DmaChannelMap);//载入DmaChannelMap——目的是设置UART RX和UART TX分别使用哪一路DMA
	if(DMA_BlockConfig(PERIPHERAL_ID_UART0_TX)!= DMA_OK)//设置DMA Block模式
	{
	   DBG("\nDma Block Mode Set Failure!\n");
	   return;
	}
	DMA_BlockBufSet(PERIPHERAL_ID_UART0_TX,(void*)DmaTxFifo,24);//设置要Block模式用的FIFO
	DMA_InterruptFunSet(PERIPHERAL_ID_UART0_TX, DMA_DONE_INT, DmaBlockInterruptUart0Tx);//设置中断函数入口
	DMA_ChannelEnable(PERIPHERAL_ID_UART0_TX);//使能用于UART1_TX的DMA通道（将TX和DmaChannel连起来。对应使用哪一路硬件DMA在DmaChannelMap中已配置好）
	DMA_InterruptEnable(PERIPHERAL_ID_UART0_TX,DMA_DONE_INT,1);//设置需要用到的DMA中断，Block模式使用的是DMA_DONE_INT中断
	DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, buf, 24);

	UARTS_IOCTL(0, UART_IOCTL_DMA_TX_EN, 1);//使能UART TX

	//block send step 4:开始传输数据
	//UART1_DMA_SendDataStart(DmaTxBuf,0);//由于是Block模式，水位中断的阀值不需要设置，填0即可

	//测试用：配置UART RX MCU中断模式
	NVIC_EnableIRQ(UART0_IRQn);//注意中断入口函数是否设置
	UARTS_IOCTL(0,UART_IOCTL_RXINT_SET, 1);
	UARTS_IOCTL(0,UART_IOCTL_RXINT_CLR, 1);
	UARTS_IOCTL(0,UART_IOCTL_RXFIFO_CLR, 1);
    while(1)
    {
    	//Block发送的多BUF切换示例
    	switch(flag)
    	{
    	case '0':
    		DBG("\n\n[Select send buf ]:\n\t");
    		DMA_BlockBufSet(PERIPHERAL_ID_UART0_TX,(void*)buf,24);//重新配置发送Buf
    		DMA_ChannelEnable(PERIPHERAL_ID_UART0_TX);//使能DMA Channel
    		UARTS_IOCTL(0, UART_IOCTL_DMA_TX_EN, 1);//使能TX
    		DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, buf, 0);//开始Block 发送数据
    		flag = 0;
    		break;
    	case '1':
    		DBG("\n\n[Select send buf2]:\n\t");
    		DMA_BlockBufSet(PERIPHERAL_ID_UART0_TX,(void*)buf1,24);
    		DMA_ChannelEnable(PERIPHERAL_ID_UART0_TX);
    		UARTS_IOCTL(0, UART_IOCTL_DMA_TX_EN, 1);
    		DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, buf1, 0);//开始Block 发送数据
    		flag = 0;
    		break;
    	case '2':
    		DBG("\n\n[Select send DmaTxBuf]:\n\t");
    		DMA_BlockBufSet(PERIPHERAL_ID_UART0_TX,(void*)DmaTxBuf,200);
    		DMA_ChannelEnable(PERIPHERAL_ID_UART0_TX);
    		UARTS_IOCTL(0, UART_IOCTL_DMA_TX_EN, 1);
    		DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, DmaTxBuf, 0);//开始Block 发送数据
    		flag = 0;
    		break;
    	case '3':
    		DBG("\n\n[Select send DmaTxBuf1]:\n\t");
    		DMA_BlockBufSet(PERIPHERAL_ID_UART0_TX,(void*)DmaTxBuf1,512);
    		DMA_ChannelEnable(PERIPHERAL_ID_UART0_TX);
    		UARTS_IOCTL(0, UART_IOCTL_DMA_TX_EN, 1);
    		DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, DmaTxBuf1, 0);//开始Block 发送数据
    		flag = 0;
    		break;
    	}
    }
}

void Example_UART_DMA_Circular_Int_Recv(void)
{
	int echo;
	//说明：Example 默认使用UART0的PORT 0口作为示例;

	//step 0:确认UART的时钟是否开启
	//step 1:配置GPIO复用
	TGPIO_UartRxIoConfig(UART_PORT0,1);//RX A5
	TGPIO_UartTxIoConfig(UART_PORT0,1);//TX A6

	//step 2:配置UART参数
	UARTS_Init(0,2000000,8,0,1);//UART模块初始化

	NVIC_EnableIRQ(UART0_IRQn);//注意中断入口函数是否设置
	UARTS_IOCTL(0,UART_IOCTL_RXINT_SET, 1);
	UARTS_IOCTL(0,UART_IOCTL_RXINT_CLR, 1);
	UARTS_IOCTL(0,UART_IOCTL_RXFIFO_CLR, 1);

	//step 2:配置DMA参数
	DMA_ChannelAllocTableSet(DmaChannelMap);//载入DmaChannelMap——目的是设置UART RX和UART TX分别使用哪一路DMA
	if(DMA_CircularConfig(PERIPHERAL_ID_UART0_RX, DMA_TST_BUF_LEN, (void*)DmaRxBuf, sizeof(DmaRxBuf)) != DMA_OK)//配置Circular模式
	{
		return;
	}
	//DMA_CircularThresholdLenSet(PERIPHERAL_ID_UART1_RX, DMA_TST_BUF_LEN);//这个函数可以配置水位
	DMA_InterruptFunSet(PERIPHERAL_ID_UART0_RX, DMA_THRESHOLD_INT, DmaInterruptUart0Rx);//配置水位中断函数入口
	DMA_InterruptEnable(PERIPHERAL_ID_UART0_RX, DMA_THRESHOLD_INT, 1);//使能水位中断
	DMA_ChannelEnable(PERIPHERAL_ID_UART0_RX);//使能DMA通道

	//step 3:使能DMA_RX
	UARTS_IOCTL(0,UART_IOCTL_DMA_RX_EN, 1);//使能DMA RX

	//测试用： 初始化循环buf
	Buf_init(&g_dma_ring_buf, BuartRGBuf, sizeof(BuartRGBuf));
	clean_ring_buffer(&g_dma_ring_buf);

	DBG("\n***** you will receive what you have sent *****\n");
	while(1)
	{
		echo=0;
		Buf_GetBytesAvail(&g_dma_ring_buf, echo);
		Buf_Pop_Multi(&g_dma_ring_buf, crc_check_buf, echo, 0);
		if(echo>0)
		{
			UARTS_Send(0,crc_check_buf,echo,100);//接收到的数据放入循环部分，在这里再发送出来
		}
	}
}

void Example_UART_DMA_Circular_NoInt_Recv(void)
{
	int i,Echo;

	//说明：Example 默认使用UART0的PORT 0口作为示例;

	//step 0:确认UART的时钟是否开启
	//step 1:配置GPIO复用
	TGPIO_UartRxIoConfig(UART_PORT0,1);//RX A5
	TGPIO_UartTxIoConfig(UART_PORT0,1);//TX A6

	//step 2:配置UART参数
	UARTS_Init(0,2000000,8,0,1);//UART模块初始化

	//step 3:配置DMA参数
	DMA_ChannelAllocTableSet(DmaChannelMap);//载入DmaChannelMap——目的是设置UART RX和UART TX分别使用哪一路DMA
	DMA_ChannelDisable(PERIPHERAL_ID_UART0_RX);
	DMA_CircularConfig(PERIPHERAL_ID_UART0_RX,sizeof(DmaRxBuf)/2,DmaRxBuf,sizeof(DmaRxBuf));
	DMA_ChannelEnable(PERIPHERAL_ID_UART0_RX);

	//step 4:使能RX
	UARTS_IOCTL(0,UART_IOCTL_DMA_RX_EN, 1);

	DBG("\n***** you will receive what you have sent *****\n");
	while(1)
	{
		//数据处理示例：
		Echo = DMA_CircularDataLenGet(PERIPHERAL_ID_UART0_RX);
		if(Echo>0)
		{
			DMA_CircularDataGet(PERIPHERAL_ID_UART0_RX,DmaRxBuf,Echo);
			for(i=0;i<Echo;i++)
			{
				DBG("%c",DmaRxBuf[i]);//展示此次收到的数据
			}
		}
	}
}
void Example_UART_DMA_Circular_Noint_Send(void)
{
	uint8_t buf[] = "0123456789abcdef";
	memset(DmaTxBuf,0x38,512);
	DBG("\n***** Select 0 1 2  *****\n");
	DBG("\n***** to choose different send data length *****\n");
	//说明：Example 默认使用UART0的PORT 0口作为示例;

	//step 0:确认UART的时钟是否开启
	//step 1:配置GPIO复用
	TGPIO_UartRxIoConfig(UART_PORT0,1);//RX A5
	TGPIO_UartTxIoConfig(UART_PORT0,1);//TX A6

	//step 2:配置UART参数
	UARTS_Init(UART_PORT0,2000000,8,0,1);//UART模块初始化

	//step 3:配置DMA参数
	DMA_ChannelAllocTableSet(DmaChannelMap);//载入DmaChannelMap——目的是设置UART RX和UART TX分别使用哪一路DMA
	UARTS_DMA_TxInit(UART_PORT0, (void*)DmaTxBuf, sizeof(DmaTxBuf), DMA_TST_BUF_LEN, DmaIntTxFunArry[UART_PORT0]);//配置

	//step 4:开始传输数据
	UARTS_DMA_SendDataStart(UART_PORT0,DmaTxBuf,256);

	//配置UART RX（用于演示）
	//TGPIO_UartRxIoConfig(UART_PORT0,1);//A5
	NVIC_EnableIRQ(UART0_IRQn);
	UARTS_IOCTL(0,UART_IOCTL_RXINT_SET, 1);
	UARTS_IOCTL(0,UART_IOCTL_RXINT_CLR, 1);
	UARTS_IOCTL(0,UART_IOCTL_RXFIFO_CLR, 1);

	while(1)
	{
		//数据处理示例：变换水位长度  切换发送Buf
		switch(flag)
		{
		case '0':
			DBG("\n\n[Select send buf]:\n\t");
			DMA_CircularConfig(PERIPHERAL_ID_UART0_TX,0,(void*)buf,16);//重新配置发送Buf
			DMA_ChannelEnable(PERIPHERAL_ID_UART0_TX);//使能DMA Channel
			UARTS_IOCTL(0, UART_IOCTL_DMA_TX_EN, 1);//使能TX
			//UARTS_DMA_SendDataStart(UART_PORT1,buf,32);//开始发送数据  发送长度超过BufLen(示例中是24），则发送不会超过24
			DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, (void*)buf, 32);
			flag = 0;
			break;
		case '1':
			DBG("\n\n[Select send buf]:\n\t");
			DMA_CircularConfig(PERIPHERAL_ID_UART0_TX,0,(void*)buf,16);//重新配置发送Buf
			DMA_ChannelEnable(PERIPHERAL_ID_UART0_TX);//使能DMA Channel
			UARTS_IOCTL(0, UART_IOCTL_DMA_TX_EN, 1);//使能TX
			//UARTS_DMA_SendDataStart(UART_PORT1,buf,10);//开始发送数据  发送长度小于BufLen(示例中是10）
			DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, (void*)buf, 10);
			flag = 0;
			break;
		case '2':
			DBG("\n\n[Select send buf]:\n\t");
			DMA_CircularConfig(PERIPHERAL_ID_UART0_TX,0,(void*)buf,16);//重新配置发送Buf
			DMA_ChannelEnable(PERIPHERAL_ID_UART0_TX);//使能DMA Channel
			UARTS_IOCTL(0, UART_IOCTL_DMA_TX_EN, 1);//使能TX
			//UARTS_DMA_SendDataStart(UART_PORT1,buf,3);//开始发送数据  发送长度小于BufLen(示例中是3）
			DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, (void*)buf, 3);
			flag = 0;
			break;
		}
	}
}

void Example_UART_DMA_Circular_Int_Send(void)
{
	memset(DmaTempBuf,0x30,512);
	memset(DmaTempBuf1,0x31,512);
	//配置UART RX（用于演示）
//	TGPIO_UartRxIoConfig(UART_PORT1,0);//A9
//	NVIC_EnableIRQ(UART1_IRQn);
//	UARTS_IOCTL(0,UART_IOCTL_RXINT_SET, 1);
//	UARTS_IOCTL(0,UART_IOCTL_RXINT_CLR, 1);
//	UARTS_IOCTL(0,UART_IOCTL_RXFIFO_CLR, 1);

	//说明：Example 默认使用UART0的PORT 0口作为示例;

	//step 0:确认UART的时钟是否开启
	//step 1:配置GPIO复用
	//TGPIO_UartRxIoConfig(UART_PORT0,1);//RX A5
	TGPIO_UartTxIoConfig(UART_PORT0,1);//TX A6

	//step 2:配置UART参数
	UARTS_Init(UART_PORT0,2000000,8,0,1);//UART模块初始化

	//step 3:配置DMA参数
	DMA_ChannelDisable(PERIPHERAL_ID_UART0_TX);
	DMA_ChannelAllocTableSet(DmaChannelMap);//载入DmaChannelMap——目的是设置UART RX和UART TX分别使用哪一路DMA
	if(DMA_CircularConfig(PERIPHERAL_ID_UART0_TX, 0, DmaTxFifo, sizeof(DmaTxFifo)) != DMA_OK)
	{
		return;
	}
	DMA_CircularThresholdLenSet(PERIPHERAL_ID_UART0_TX, 256);
	DMA_InterruptFunSet(PERIPHERAL_ID_UART0_TX, DMA_THRESHOLD_INT, DmaInterruptUart0Tx);
	DMA_InterruptEnable(PERIPHERAL_ID_UART0_TX, DMA_THRESHOLD_INT, 1);
	DMA_CircularDataPut(PERIPHERAL_ID_UART0_TX, DmaTempBuf, sizeof(DmaTempBuf)/2);//注意发送Buf要和FiFo尽量区别

	DMA_ChannelEnable(PERIPHERAL_ID_UART0_TX);
	//step 4:使能DMA_TX
	UARTS_IOCTL(0,UART_IOCTL_DMA_TX_EN, 1);

	//数据处理示例：
	//见中断处理函数DmaInterruptUart1Tx
	while(1);
}

__attribute__((section(".driver.isr")))
void UART1_Interrupt(void)
{
    if(UARTS_IOCTL(1,UART_IOCTL_RXSTAT_GET, 1) & 0x01)
    {
    	UARTS_RecvByte(1,&rxdata);
        UARTS_IOCTL(1,UART_IOCTL_RXINT_CLR, 1);
        flag = rxdata;

        //演示：接收到的发送
        //DBG("\n[Recieved data]:\n");
        UARTS_SendByte(1,rxdata); //DBG("%c",rxdata);

		//实际使用中，要在中断处理函数外面处理数据，否则可能影响效率。
		//（注意:RX FIFO为4Bytes）这种模式下，由于Cache的存在，第一次执行中断入口函数中的代码效率会比后面低
	}

	if( UARTS_IOCTL(1,UART_IOCTL_RX_ERR_INT_GET, 0)
		&& (UARTS_IOCTL(1,UART_IOCTL_RXSTAT_GET, 0) & 0x1C))//ERROR INT
	{
		UARTS_IOCTL(1,UART_IOCTL_RX_ERR_CLR, 1);
//		UARTS_IOCTL(0,UART_IOCTL_TXFIFO_CLR, 1);
		UARTS_IOCTL(1,UART_IOCTL_RXFIFO_CLR, 1);
		DBG("\nUART_IOCTL_RX_ERR_INT_GET\n");
    }

//    if(UARTS_IOCTL(0,UART_IOCTL_TXSTAT_GET, 0)) { //TX Finished INT
//    	UARTS_IOCTL(0,UART_IOCTL_TXINT_CLR, 1);
//    	//DBG("\nUART_IOCTL_TXSTAT_GET\n");
//    }

    if(UARTS_IOCTL(1,UART_IOCTL_RXSTAT_GET, 0) & 0x02) { //OVERTIME INT
    	UARTS_IOCTL(1,UART_IOCTL_OVERTIME_CLR, 1);
    	DBG("\nUART_IOCTL_OVERTIME_GET\n");
    }
}

int main(void)
{
  	uint8_t Key;

    Chip_Init(1);
    __c_init_rom();
    WDG_Disable();
    Clock_Config(1, 24000000);
    Clock_HOSCCurrentSet(15);  // 加大了晶体的偏置电流
    Clock_PllLock(240 * 1000); // 240M频率
    Clock_APllLock(240 * 1000);
    Clock_Module1Enable(ALL_MODULE1_CLK_SWITCH);
    Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
    Clock_Module3Enable(ALL_MODULE3_CLK_SWITCH);
    Clock_SysClkSelect(PLL_CLK_MODE);
    Clock_UARTClkSelect(PLL_CLK_MODE);
    Clock_HOSCCurrentSet(5);

   // Remap_InitTcm(0x40000, 12);
    SpiFlashInit(80000000, MODE_4BIT, 0, 1);

    GPIO_PortAModeSet(GPIOA10, 5);// UART1 TX
    GPIO_PortAModeSet(GPIOA9, 1);// UART1 RX
    DbgUartInit(1, 2000000, 8, 0, 1);


	DBG("****************************************************************\n");
	DBG("               UART Example MVSilicon  \n");
	DBG("****************************************************************\n");

	APP_DBG("Driver Version: %s\n", GetLibVersionDriver());

	DBG("Select an example:\n");
	DBG("0:: MCU with no interrupt\n");
	DBG("1:: MCU with interrupt\n");
	DBG("2:: DMA Circular send with interrupt\n");
	DBG("3:: DMA Circular send with no interrupt\n");
	DBG("4:: DMA Circular receive with interrupt\n");
	DBG("5:: DMA Circular receive with no interrupt\n");
	DBG("6:: DMA Block send\n");
	DBG("7:: OVERTIME example\n");
	Key = WaitDatum1Ever();//Key = 1;

	switch(Key)
	{
	case 0:
		//MCU mode with no interrupt receive
		DBG("Selected: MCU with no interrupt\n");
		Example_UART_MCU_NonInt();
		break;
	case 1:
		// MCU mode with interrupt receive
		DBG("Selected:: MCU with interrupt\n");
		Example_UART_MCU_Int();
		break;
	case 2:
		// DMA circular mode send with interrupt
		DBG("DMA Circular send with interrupt example\n");
		Example_UART_DMA_Circular_Int_Send();
		break;
	case 3:
		// DMA circular mode send with no interrupt
		DBG("DMA Circular send with no interrupt example\n");
		Example_UART_DMA_Circular_Noint_Send();
		break;
	case 4:
		// DMA circular mode receive with interrupt
		DBG("DMA Circular receive with interrupt example\n");
		Example_UART_DMA_Circular_Int_Recv();
		break;
	case 5:
		// DMA circular mode receive with no interrupt
		DBG("DMA Circular receive with no interrupt example\n");
		Example_UART_DMA_Circular_NoInt_Recv();
		break;
	case 6:
		//  DMA block mode send
		DBG("DMA Block send example\n");
		Example_UART_DMA_Block_Send();
		break;
	case 7:
		DBG("OVERTIME example\n");
		NVIC_EnableIRQ(UART1_IRQn);//注意中断入口函数是否设置
		UARTS_IOCTL(1,UART_IOCTL_RXINT_SET, 1);
		UARTS_IOCTL(1,UART_IOCTL_RXINT_CLR, 1);
		UARTS_IOCTL(1,UART_IOCTL_RXFIFO_CLR, 1);
		UARTS_IOCTL(1,UART_IOCTL_OVERTIME_SET, 1);
		//配置over time的时间。默认是0x1000，可以根据实际需求进行配置。
		//rx overtime timing cnt value, its unit is baud_rate.
		//eg. overtime_nume is 1000, if during the 1000 baud_rate time, has no new data received, an overtime interrput will generate.
		UARTS_IOCTL(1,UART_IOCTL_OVERTIME_NUM, 0x10);
		break;
	default:
		break;
	}

	while(1);
	return -1;
}



