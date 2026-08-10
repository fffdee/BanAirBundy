/**
 *****************************************************************************
 * @file     otg_device_cdc.c
 * @author   BG Card Team
 * @version  V1.0.0
 * @date     15-December-2025
 * @brief    CDC serial port device implementation
 *****************************************************************************
 * @attention
 *
 * CDC (Communication Device Class) implementation for USB virtual COM port
 * 
 *****************************************************************************
 */

#include <string.h>
#include "type.h"
#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
#include "otg_device_cdc.h"

// CDC Device Instance
UsbCDC_t UsbCDC;

// External variables
extern uint8_t Setup[];
extern uint8_t Request[];

/* LoadFIFOData always memcpy()'s; NULL+len0 is UB on this libc → stall/crash.
 * Use a real buffer for ZLP status stages. */
static uint8_t s_ep0_zlp;

/* Set after SET_CONFIGURATION status ZLP; cleared on bus reset.
 * Main loop defers OTG_DeviceCDC_Init() until this is set. */
volatile uint8_t g_usb_configured;

/* #region agent log — set in EP0 CDC path, printed from task (no UART in EP0) */
#define D503_CDC_SET_CTRL   (1u << 0)
#define D503_CDC_SET_LINE   (1u << 1)
#define D503_CDC_GET_LINE   (1u << 2)
#define D503_CDC_OTHER      (1u << 3)
volatile uint32_t g_d503_cdc_evt;
volatile uint32_t g_d503_cdc_baud;
volatile uint32_t g_d503_cdc_dtr_rts;
volatile uint32_t g_d503_cdc_serr;
volatile uint32_t g_d503_cdc_last_req;
/* #endregion */

// Forward declaration of interrupt callback
void OnDeviceCDC_BulkOutReceived(void);

/**
 * @brief  Initialize CDC device
 */
bool OTG_DeviceCDC_Init(void)
{
    memset(&UsbCDC, 0, sizeof(UsbCDC_t));
    
    // Default line coding: 115200 bps, 1 stop bit, no parity, 8 data bits
    UsbCDC.LineCoding.dwDTERate = 115200;
    UsbCDC.LineCoding.bCharFormat = 0;  // 1 stop bit
    UsbCDC.LineCoding.bParityType = 0;  // No parity
    UsbCDC.LineCoding.bDataBits = 8;    // 8 data bits
    
    // Initialize control line state
    UsbCDC.ControlLineState.DTR = 0;
    UsbCDC.ControlLineState.RTS = 0;
    UsbCDC.IsConnected = 0;  // 初始状态为未连接
    
    // Initialize buffers
    UsbCDC.RxHead = 0;
    UsbCDC.RxTail = 0;
    UsbCDC.RxCount = 0;
    
    UsbCDC.TxHead = 0;
    UsbCDC.TxTail = 0;
    UsbCDC.TxCount = 0;
    UsbCDC.TxBusy = 0;
    
    UsbCDC.InitOk = 1;

    /* Bootloader: do NOT enable Bulk OUT IRQ. UsbInterrupt can race with EP0
     * ControlSend (EP index 0x0e / CSR). Poll in OTG_DeviceCDC_Task instead. */
    return TRUE;
}

/**
 * @brief  DeInitialize CDC device
 */
bool OTG_DeviceCDC_DeInit(void)
{
    UsbCDC.InitOk = 0;
    memset(&UsbCDC, 0, sizeof(UsbCDC_t));
    return TRUE;
}

/**
 * @brief  CDC class specific request handler
 */
void OTG_DeviceCDC_Request(void)
{
    uint8_t bRequest = Setup[1];
    OTG_DEVICE_ERR_CODE serr = DEVICE_NONE_ERR;

    // #region agent log
    g_d503_cdc_last_req = bRequest;
    // #endregion agent log

    switch (bRequest)
    {
        case CDC_SET_LINE_CODING:
            UsbCDC.LineCoding.dwDTERate = Request[0] | (Request[1] << 8) |
                                           (Request[2] << 16) | (Request[3] << 24);
            UsbCDC.LineCoding.bCharFormat = Request[4];
            UsbCDC.LineCoding.bParityType = Request[5];
            UsbCDC.LineCoding.bDataBits = Request[6];
            UsbCDC.IsConnected = 1;
            serr = OTG_DeviceControlSend(&s_ep0_zlp, 0, 10);
            // #region agent log
            g_d503_cdc_evt |= D503_CDC_SET_LINE;
            g_d503_cdc_baud = UsbCDC.LineCoding.dwDTERate;
            g_d503_cdc_serr = (uint32_t)serr;
            // #endregion agent log
            break;

        case CDC_GET_LINE_CODING:
            {
                uint8_t lineCodingBuf[7];
                lineCodingBuf[0] = (uint8_t)(UsbCDC.LineCoding.dwDTERate & 0xFF);
                lineCodingBuf[1] = (uint8_t)((UsbCDC.LineCoding.dwDTERate >> 8) & 0xFF);
                lineCodingBuf[2] = (uint8_t)((UsbCDC.LineCoding.dwDTERate >> 16) & 0xFF);
                lineCodingBuf[3] = (uint8_t)((UsbCDC.LineCoding.dwDTERate >> 24) & 0xFF);
                lineCodingBuf[4] = UsbCDC.LineCoding.bCharFormat;
                lineCodingBuf[5] = UsbCDC.LineCoding.bParityType;
                lineCodingBuf[6] = UsbCDC.LineCoding.bDataBits;
                serr = OTG_DeviceControlSend(lineCodingBuf, 7, 10);
                // #region agent log
                g_d503_cdc_evt |= D503_CDC_GET_LINE;
                g_d503_cdc_baud = UsbCDC.LineCoding.dwDTERate;
                g_d503_cdc_serr = (uint32_t)serr;
                // #endregion agent log
            }
            break;

        case CDC_SET_CONTROL_LINE_STATE:
            UsbCDC.ControlLineState.DTR = (Setup[2] & 0x01) ? 1 : 0;
            UsbCDC.ControlLineState.RTS = (Setup[2] & 0x02) ? 1 : 0;
            serr = OTG_DeviceControlSend(&s_ep0_zlp, 0, 10);
            // #region agent log
            g_d503_cdc_evt |= D503_CDC_SET_CTRL;
            g_d503_cdc_dtr_rts = (uint32_t)Setup[2] | ((uint32_t)Setup[3] << 8);
            g_d503_cdc_serr = (uint32_t)serr;
            // #endregion agent log
            break;

        case CDC_SEND_BREAK:
            serr = OTG_DeviceControlSend(&s_ep0_zlp, 0, 10);
            // #region agent log
            g_d503_cdc_evt |= D503_CDC_OTHER;
            g_d503_cdc_serr = (uint32_t)serr;
            // #endregion agent log
            break;

        case CDC_SEND_ENCAPSULATED_COMMAND:
            // #region agent log
            g_d503_cdc_evt |= D503_CDC_OTHER;
            // #endregion agent log
            break;

        case CDC_GET_ENCAPSULATED_RESPONSE:
            serr = OTG_DeviceControlSend(Request, Setup[6], 3);
            // #region agent log
            g_d503_cdc_evt |= D503_CDC_OTHER;
            g_d503_cdc_serr = (uint32_t)serr;
            // #endregion agent log
            break;

        default:
            // #region agent log
            g_d503_cdc_evt |= D503_CDC_OTHER;
            // #endregion agent log
            break;
    }
}

/**
 * @brief  USB Bulk OUT中断回调函数 (当USB硬件接收到数据时自动调用)
 * @note   此函数在USB硬件中断上下文中执行，需要快速返回
 */
void OnDeviceCDC_BulkOutReceived(void)
{
    uint8_t tmpBuf[CDC_DATA_FS_OUT_PACKET_SIZE];
    uint16_t i;
    uint32_t actualLen = 0;
    
    // 从USB硬件FIFO读取接收到的数据
    OTG_DEVICE_ERR_CODE ret = OTG_DeviceBulkReceive(DEVICE_CDC_DATA_OUT_EP, tmpBuf, CDC_DATA_FS_OUT_PACKET_SIZE, &actualLen, 0);
    
    if(ret == DEVICE_NONE_ERR && actualLen > 0)
    {
        // 快速将数据复制到环形缓冲区
        for(i = 0; i < actualLen; i++)
        {
            if(UsbCDC.RxCount < CDC_RX_BUFFER_SIZE)
            {
                UsbCDC.RxBuffer[UsbCDC.RxHead] = tmpBuf[i];
                UsbCDC.RxHead = (UsbCDC.RxHead + 1) % CDC_RX_BUFFER_SIZE;
                UsbCDC.RxCount++;
            }
            else
            {
                // 缓冲区溢出（静默丢弃，避免在中断中打印）
                break;
            }
        }
    }
    // 中断模式下无需打印错误信息，硬件会自动处理
}

/**
 * @brief  CDC data received callback (保留兼容，但不再主动轮询)
 * @note   此函数已废弃，由中断回调OnDeviceCDC_BulkOutReceived取代
 */
void OTG_DeviceCDC_DataReceived(void)
{
    // 此函数仅为保持兼容性，实际接收由中断处理
    // 不再主动轮询USB，避免产生error 001错误
}

/**
 * @brief  CDC data transmitted callback (called when data transmission is complete)
 */
void OTG_DeviceCDC_DataTransmitted(void)
{
    UsbCDC.TxBusy = 0;
    
    // If there's more data in TX buffer, send it
    if(UsbCDC.TxCount > 0)
    {
        OTG_DeviceCDC_FlushTx();
    }
}

/**
 * @brief  Send data through CDC
 * @param  buf: pointer to data buffer
 * @param  len: data length
 * @return number of bytes sent (queued)
 */
uint16_t OTG_DeviceCDC_Send(uint8_t *buf, uint16_t len)
{
    uint16_t i;
    uint16_t count = 0;
    
    if(!UsbCDC.InitOk || buf == NULL || len == 0)
    {
        return 0;
    }
    
    // Copy data to TX buffer
    for(i = 0; i < len; i++)
    {
        if(UsbCDC.TxCount < CDC_TX_BUFFER_SIZE)
        {
            UsbCDC.TxBuffer[UsbCDC.TxHead] = buf[i];
            UsbCDC.TxHead = (UsbCDC.TxHead + 1) % CDC_TX_BUFFER_SIZE;
            UsbCDC.TxCount++;
            count++;
        }
        else
        {
            // Buffer full
            break;
        }
    }
    
    // Trigger transmission if not busy
    if(!UsbCDC.TxBusy && UsbCDC.TxCount > 0)
    {
        OTG_DeviceCDC_FlushTx();
    }
    
    return count;
}

/**
 * @brief  Receive data from CDC
 * @param  buf: pointer to data buffer
 * @param  len: maximum data length to receive
 * @return number of bytes received
 */
uint16_t OTG_DeviceCDC_Receive(uint8_t *buf, uint16_t len)
{
    uint16_t i;
    uint16_t count = 0;
    
    if(!UsbCDC.InitOk || buf == NULL || len == 0)
    {
        return 0;
    }
    
    // 直接从缓冲区读取数据，不触发USB接收（由Task主动轮询）
    for(i = 0; i < len && UsbCDC.RxCount > 0; i++)
    {
        buf[i] = UsbCDC.RxBuffer[UsbCDC.RxTail];
        UsbCDC.RxTail = (UsbCDC.RxTail + 1) % CDC_RX_BUFFER_SIZE;
        UsbCDC.RxCount--;
        count++;
    }
    
    return count;
}

/**
 * @brief  Get available bytes in CDC RX buffer
 * @return number of available bytes
 */
uint16_t OTG_DeviceCDC_GetRxCount(void)
{
    return UsbCDC.RxCount;
}

/**
 * @brief  Peek at the next byte in CDC RX buffer without consuming it.
 *         Used by SOF dispatchers to check protocol start byte before
 *         committing to consume the data.
 * @param  byte: pointer to receive the peeked byte
 * @return 1 if a byte was peeked, 0 if buffer empty
 */
uint16_t OTG_DeviceCDC_PeekByte(uint8_t *byte)
{
    if(!UsbCDC.InitOk || byte == NULL || UsbCDC.RxCount == 0)
    {
        return 0;
    }
    *byte = UsbCDC.RxBuffer[UsbCDC.RxTail];
    return 1;
}

/**
 * @brief  Get available space in CDC TX buffer
 * @return number of available bytes
 */
uint16_t OTG_DeviceCDC_GetTxFreeSpace(void)
{
    return (CDC_TX_BUFFER_SIZE - UsbCDC.TxCount);
}

/**
 * @brief  Flush CDC TX buffer (send all pending data)
 * @return TRUE if success
 */
bool OTG_DeviceCDC_FlushTx(void)
{
    uint8_t tmpBuf[CDC_DATA_FS_IN_PACKET_SIZE];
    uint16_t len = 0;
    uint16_t i;
    
    if(!UsbCDC.InitOk || UsbCDC.TxBusy || UsbCDC.TxCount == 0)
    {
        return FALSE;
    }
    
    // Determine how many bytes to send
    len = UsbCDC.TxCount;
    if(len > CDC_DATA_FS_IN_PACKET_SIZE)
    {
        len = CDC_DATA_FS_IN_PACKET_SIZE;
    }
    
    // Copy data from TX buffer
    for(i = 0; i < len; i++)
    {
        tmpBuf[i] = UsbCDC.TxBuffer[UsbCDC.TxTail];
        UsbCDC.TxTail = (UsbCDC.TxTail + 1) % CDC_TX_BUFFER_SIZE;
    }
    UsbCDC.TxCount -= len;
    
    // Mark as busy
    UsbCDC.TxBusy = 1;
    
    // Send data through USB endpoint (使用Bulk IN端点)
    // 超时1000ms: 给主机充足时间读取数据，避免因主机轮询延迟而丢包
    OTG_DeviceBulkSend(DEVICE_CDC_DATA_IN_EP, tmpBuf, len, 1000);
    
    // 发送完成后清除busy标志（同步模式）
    UsbCDC.TxBusy = 0;
    
    // 如果还有数据，继续发送
    if(UsbCDC.TxCount > 0)
    {
        OTG_DeviceCDC_FlushTx();
    }
    
    return TRUE;
}

/**
 * @brief  Flush CDC RX buffer (clear all data)
 * @return TRUE if success
 */
bool OTG_DeviceCDC_FlushRx(void)
{
    if(!UsbCDC.InitOk)
    {
        return FALSE;
    }
    
    UsbCDC.RxHead = 0;
    UsbCDC.RxTail = 0;
    UsbCDC.RxCount = 0;
    
    return TRUE;
}

/**
 * @brief  CDC task - 处理接收到的数据（数据接收由中断自动完成）
 * @note   使用中断接收后，此Task只需处理缓冲区中的数据，无需轮询USB
 *         数据处理由Shell命令行系统接管
 */
void OTG_DeviceCDC_Task(void)
{
    uint8_t tmpBuf[CDC_DATA_FS_OUT_PACKET_SIZE];
    uint32_t actualLen = 0;
    uint16_t i;
    OTG_DEVICE_ERR_CODE ret;

    if (!UsbCDC.InitOk)
        return;

    /* Poll Bulk OUT (no IRQ) — safe alongside EP0 RequestProcess */
    ret = OTG_DeviceBulkReceive(DEVICE_CDC_DATA_OUT_EP, tmpBuf,
                                CDC_DATA_FS_OUT_PACKET_SIZE, &actualLen, 0);
    if (ret == DEVICE_NONE_ERR && actualLen > 0) {
        for (i = 0; i < actualLen; i++) {
            if (UsbCDC.RxCount < CDC_RX_BUFFER_SIZE) {
                UsbCDC.RxBuffer[UsbCDC.RxHead] = tmpBuf[i];
                UsbCDC.RxHead = (UsbCDC.RxHead + 1) % CDC_RX_BUFFER_SIZE;
                UsbCDC.RxCount++;
            } else {
                break;
            }
        }
    }
}

/**
 * @brief  Send a single character through CDC
 * @param  ch: character to send
 * @return number of bytes sent
 */
uint16_t OTG_DeviceCDC_SendChar(uint8_t ch)
{
    return OTG_DeviceCDC_Send(&ch, 1);
}

/**
 * @brief  Get a single character from CDC RX buffer
 * @return character received, or 0 if no data
 */
uint8_t OTG_DeviceCDC_GetChar(void)
{
    uint8_t ch = 0;
    OTG_DeviceCDC_Receive(&ch, 1);
    return ch;
}

/**
 * @brief  Flush CDC RX buffer (wrapper for compatibility)
 * @return TRUE if success
 */
bool OTG_DeviceCDC_FlushRxBuffer(void)
{
    return OTG_DeviceCDC_FlushRx();
}

/**
 * @brief  Flush CDC TX buffer (wrapper for compatibility)
 * @return TRUE if success
 */
bool OTG_DeviceCDC_FlushTxBuffer(void)
{
    return OTG_DeviceCDC_FlushTx();
}
