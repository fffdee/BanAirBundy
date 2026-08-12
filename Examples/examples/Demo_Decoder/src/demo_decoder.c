/**
 **************************************************************************************
 * @file    demo_decoder.c
 * @brief   decoder example
 *
 * @author  Castle
 * @version V1.0.0
 *
 * $Created: 2018-01-04 19:17:00$
 *
 * @Copyright (C) 2017, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <stdlib.h>
#include <nds32_intrinsic.h>
#include <string.h>
#include "watchdog.h"
#include "sys.h"
#include "gpio.h"
#include "uarts.h"
#include "uarts_interface.h"
#include "dac_interface.h"
#include "type.h"
#include "debug.h"
#include "timeout.h"
#include "clk.h"
#include "dma.h"
#include "timer.h"
#include "spi_flash.h"
#include "remap.h"
#include "irqn.h"
#include "sdio.h"
#include "sd_card.h"
#include "dac.h"
#include "chip_info.h"
#include "ff.h"
#include "ffpresearch.h"

#include "audio_decoder_api.h"
#include "typedefine.h"
#include "sd_card.h"

#define	CFG_RES_CARD_GPIO				SDIO_A15_A16_A17
#define SDIO_Clk_Disable				SDIO_ClkDisable
#define SDIO_Clk_Eable					SDIO_ClkEnable
#define CARD_DETECT_GPIO				GPIOA16
#define CARD_DETECT_GPIO_IN				GPIO_A_IN
#define CARD_DETECT_BIT_MASK			GPIOA16
#define CARD_DETECT_GPIO_IE				GPIO_A_IE
#define CARD_DETECT_GPIO_OE				GPIO_A_OE
#define CARD_DETECT_GPIO_PU				GPIO_A_PU
#define CARD_DETECT_GPIO_PD				GPIO_A_PD
#define CARD_DETECT_GPIO_OUT			GPIO_A_OUT

static uint8_t DmaChannelMap[] =
{
    PERIPHERAL_ID_AUDIO_DAC0_TX,
	PERIPHERAL_ID_SDIO_RX,
    255,
    255,
    255,
    255,
};


bool IsSdLink(void)
{
	bool FindCard = FALSE;
	uint8_t BackupMode;
	static uint32_t check_mask = 0x55aa;


	SDIO_Clk_Disable();
	BackupMode = GPIO_PortAModeGet(CARD_DETECT_GPIO);

	GPIO_PortAModeSet(CARD_DETECT_GPIO, 0x0);
	GPIO_RegOneBitSet(CARD_DETECT_GPIO_PU, CARD_DETECT_BIT_MASK);
	GPIO_RegOneBitClear(CARD_DETECT_GPIO_PD, CARD_DETECT_BIT_MASK);
	GPIO_RegOneBitSet(CARD_DETECT_GPIO_IE, CARD_DETECT_BIT_MASK);

	check_mask <<= 1;
	if(!GPIO_RegOneBitGet(CARD_DETECT_GPIO_IN, CARD_DETECT_BIT_MASK))
	{
		check_mask |= 0x01;
	}

	GPIO_PortAModeSet(CARD_DETECT_GPIO, BackupMode);
	SDIO_Clk_Eable(); //recover

	if((check_mask & 0xffff) == 0xffff)
		FindCard = TRUE;
	if((check_mask & 0xffff) == 0x0000)
		FindCard = FALSE;

	return FindCard;

}

int16_t vol_l = 0x200;
int16_t vol_r = 0x200;
int16_t vol_x = 0x200;


FATFS gFatfs_u;   /* File system object */
FATFS gFatfs_sd;   /* File system object */

FIL gFil;    /* File object */
FIL gFilx;    /* File object */
ff_dir gDirs;    /* Directory object */

uint32_t AudioDACBuf[1024*10] = {0};

static uint32_t decoder_buf[1024 * 50 / 4] = {0};

static AudioDecoderContext *decoder_ctn = (AudioDecoderContext *)decoder_buf;

static char current_vol[8];//disk volume like 0:/, 1:/

void Timer2Interrupt(void)
{
	Timer_InterruptFlagClear(TIMER2, UPDATE_INTERRUPT_SRC);
	OTG_PortLinkCheck();
}

char file_long_name[FF_LFN_BUF+1];
uint8_t acc_ram_blk[MAX_ACC_RAM_SIZE];
int main(void)
{
	int32_t i;
	bool u_play_flag;
	uint32_t play_samples,Total;

	uint32_t SampleRateCC = 48000;
	uint32_t DACBitWidth = 16;

	DACParamCt ct;
	ct.DACModel = DAC_Single;
	ct.DACLoadStatus = DAC_Load;
	ct.PVDDModel = PVDD33;
	ct.DACEnergyModel = DACLowEnergy;
	ct.DACVcomModel = Disable;

    Chip_Init(1);
    WDG_Disable();
    __c_init_rom();
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

    SpiFlashInit(80000000, MODE_4BIT, 0, 1);

    // BP15系列开发板启用串口，默认使用
    GPIO_PortAModeSet(GPIOA10, 5);// UART1 TX
    GPIO_PortAModeSet(GPIOA9, 1);// UART1 RX
    DbgUartInit(1, 2000000, 8, 0, 1);

    DMA_ChannelAllocTableSet(DmaChannelMap);

    DBG("\n");
    DBG("/-----------------------------------------------------\\\n");
    DBG("|                     Demo decoder                      |\n");
    DBG("|      Mountain View Silicon Technology Co.,Ltd.       |\n");
    DBG("\\-----------------------------------------------------/\n");
    DBG("\n");

    SysTickInit();


    OTG_PortSetDetectMode(1,0);

 	Timer_Config(TIMER2,1000,0);
 	Timer_Start(TIMER2);
 	NVIC_EnableIRQ(Timer2_IRQn);

 	OTG_HostFifoInit();

 	CardPortInit(CFG_RES_CARD_GPIO);

 	while(1)
 	{
		if(OTG_PortHostIsLink())
		{
			if(OTG_HostInit())
			{
				DBG("枚举MASSSTORAGE接口OK\n");
				strcpy(current_vol, "1:/");
				if(f_mount(&gFatfs_u, current_vol, 1) == 0)
				{
					DBG("U盘卡挂载到 1:/--> 成功\n");
					DBG("type = %d\n", gFatfs_u.fs_type);
					ffpresearch_init(&gFatfs_u,NULL, NULL,acc_ram_blk);
					u_play_flag = TRUE;
				}
				else
				{
					DBG("U盘卡挂载到 1:/--> 失败\n");
					continue;
				}
			}
			else
			{
				DBG("UDisk init Failed!\n");
				continue;
			}
		}
		else if(IsSdLink())
		{
			if(SDCard_Init() == 0)
			{
				DBG("SDCard Init Success!\n");
				strcpy(current_vol, "0:/");
				if(f_mount(&gFatfs_sd, current_vol, 1) == 0)
				{
					DBG("SD卡挂载到 0:/--> 成功\n");
					ffpresearch_init(&gFatfs_sd,NULL, NULL,acc_ram_blk);
					u_play_flag = FALSE;
				}
				else
				{
					DBG("SD卡挂载到 0:/--> 失败\n");
					continue;
				}
			}
			else
			{
				DBG("SD卡 init Failed!\n");
				continue;
			}
		}
		else
		{
			continue;
		}

		f_scan_vol(current_vol);

		AudioDAC_Init(&ct, SampleRateCC, DACBitWidth, (void *)AudioDACBuf, sizeof(AudioDACBuf), NULL, 0);
		Total = f_file_real_quantity();
		DBG("Total: %d\n", Total);
		for(i = 1; i <= Total; i++)
		//for(i = 199; i <= f_file_real_quantity(); i++)
		{
			FileType fileType;
			DBG("i;%d\n", (int)i);
			memset(&gFil, 0x00, sizeof(gFil));
			memset(file_long_name, 0x00, sizeof(file_long_name));
			if(FR_OK != f_open_by_num(current_vol, &gDirs, &gFil, i, file_long_name))
			{
				DBG("Open file error!\n");
				continue;
			}
			fileType = get_audio_type((TCHAR *)file_long_name);

			DBG("Song Name: %s\n", gFil.fn);
			DBG("Song Name long: %s\n", file_long_name);
			DBG("Song type: %d\n", fileType);

			if(RT_SUCCESS == audio_decoder_initialize(decoder_buf, &gFil, IO_TYPE_FILE, fileType))
			{
				DBG("decoder init OK! size=%d\n", (int)decoder_ctn->decoder_size);
				//Audio_SampleRateChange(decoder_ctn->song_info.sampling_rate);
				DACBitWidth = decoder_ctn->song_info.pcm_bit_width;
				AudioDAC_Init(&ct, decoder_ctn->song_info.sampling_rate, DACBitWidth, (void *)AudioDACBuf, sizeof(AudioDACBuf), NULL, 0);
				SampleRateCC = decoder_ctn->song_info.sampling_rate;
				play_samples = 0;

				DBG("[SONG_INFO]: ChannelCnt : %6d\n",		  (unsigned int)decoder_ctn->song_info.num_channels);
				DBG("[SONG_INFO]: SampleRate : %6d Hz\n",	  (unsigned int)decoder_ctn->song_info.sampling_rate);
				DBG("[SONG_INFO]: BitRate	 : %6d Kbps\n",   (unsigned int)decoder_ctn->song_info.bitrate / 1000);
				DBG("[SONG_INFO]: DecoderSize: %6d Bytes \n", (unsigned int)decoder_ctn->decoder_size);
				DBG("[SONG_INFO]: pcm_bit_width: %6d\n", (unsigned int)decoder_ctn->song_info.pcm_bit_width);
				DBG("[SONG_INFO]: Duration	 : %02u:%02u:%02u\n",	  (unsigned int)(decoder_ctn->song_info.duration / 1000) / 3600, (unsigned int)((decoder_ctn->song_info.duration / 1000) % 3600) / 60, (unsigned int)((decoder_ctn->song_info.duration / 1000) % 3600) % 60);
				DBG("[SONG_INFO]: IsVBR 	 : %s\n",		  decoder_ctn->song_info.vbr_flag ? "   YES" : "    NO");

				DBG("[SONG_INFO]: CHARSET	 : %6d\n", (int)decoder_ctn->song_info.char_set);
				DBG("[SONG_INFO]: TITLE 	 : %s\n", decoder_ctn->song_info.title);
				DBG("[SONG_INFO]: ARTIST	 : %s\n", decoder_ctn->song_info.artist);
				DBG("[SONG_INFO]: ALBUM 	 : %s\n", decoder_ctn->song_info.album);
				DBG("[SONG_INFO]: COMMT 	 : %s\n", decoder_ctn->song_info.comment);
				DBG("[SONG_INFO]: GENRE 	 : %s\n", decoder_ctn->song_info.genre_str);
				DBG("[SONG_INFO]: YEAR		 : %s\n", decoder_ctn->song_info.year);
				DBG("[SONG_INFO]: TRACK 	 : %6d\n", decoder_ctn->song_info.track);
				DBG("\n");

				AudioDAC_VolSet(AUDIO_DAC0, vol_l, vol_r);

				while(RT_YES == audio_decoder_can_continue(decoder_ctn))
				{
					uint8_t c;
					if(RT_SUCCESS == audio_decoder_decode(decoder_ctn))
					{
						static uint32_t play_sec, last_play_sec = 0;
//						if(SampleRateCC != decoder_ctn->song_info.sampling_rate)
//						{
//							DBG("[INFO]: Change sample rate from %u to %u Hz\n", (unsigned int)SampleRateCC, (unsigned int)decoder_ctn->song_info.sampling_rate);
//							SampleRateCC = decoder_ctn->song_info.sampling_rate;
//							Audio_SampleRateChange(decoder_ctn->song_info.sampling_rate);
//						}

						while(AudioDAC0_DataSpaceLenGet() < decoder_ctn->song_info.pcm_data_length)
						{
//							DBG("%d~%d ",AudioDAC0_DataSpaceLenGet(),decoder_ctn->song_info.pcm_data_length);
						}
						if(decoder_ctn->song_info.num_channels == 1)
						{
							uint16_t i, *one_sample = (uint16_t*)decoder_ctn->song_info.pcm_addr;
							for(i = 0; i < decoder_ctn->song_info.pcm_data_length; i++)
							{
								uint16_t osm[2];
								osm[0] = *one_sample; osm[1] = *one_sample;
								AudioDAC0_DataSet(osm, 1);
								one_sample ++;
							}
						}
						else
						{
							uint16_t i, *one_sample = (uint16_t*)decoder_ctn->song_info.pcm_addr;
							AudioDAC0_DataSet(decoder_ctn->song_info.pcm_addr, decoder_ctn->song_info.pcm_data_length);

						}
						play_samples += decoder_ctn->song_info.pcm_data_length;
						play_sec = play_samples / decoder_ctn->song_info.sampling_rate;
	//					DBG(".");
						if((play_sec % 3600) % 60 != last_play_sec)
						{
							DBG("play time: %02u:%02u:%02u\n", (unsigned int)play_sec / 3600, (unsigned int)(play_sec % 3600) / 60, (unsigned int)(play_sec % 3600) % 60);
							last_play_sec = (play_sec % 3600) % 60;
						}
					}
					else
					{
						DBG("[INFO]: ERROR %d\n", (int)decoder_ctn->error_code);
						if(u_play_flag && !OTG_PortHostIsLink())
							break;
						if(!u_play_flag && !IsSdLink())
							break;
					}

					if(UART_RecvByte(UART_PORT1,&c))
					{
						if(c == 'p')
						{
							DBG("\nNext...\n");
							break;
						}

						if(c == 'k')
						{
							if(vol_l > 10)
							{
								vol_l -= 10;vol_r -= 10;
								AudioDAC_VolSet(AUDIO_DAC0, vol_l, vol_r);
								DBG("vol-: %d\n", vol_l);
							}
							else
							{
								vol_l = 0;vol_r = 0;
								AudioDAC_VolSet(AUDIO_DAC0, vol_l, vol_r);
								DBG("vol-: %d\n", vol_l);
							}
						}

						if(c == 'l')
						{
							if(vol_l < 0xaff)
							{
								vol_l += 10;vol_r += 10;
								AudioDAC_VolSet(AUDIO_DAC0, vol_l, vol_r);
								DBG("vol+: %d\n", vol_l);
							}
						}
					}
				}
			}
			else
			{
				DBG("decoder init failed!!! %d\n", (int)audio_decoder_get_error_code(decoder_ctn));
			}

			f_close(&gFil);
		}
		DBG("Play finished!!!\n");
 	}
}
