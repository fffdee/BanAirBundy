#include <stdlib.h>
#include <nds32_intrinsic.h>
#include "type.h"
#include "debug.h"
#include "dma.h"
#include "dac.h"
#include "clk.h"
#include "audio_adc.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "usb_audio_api.h"
// 定义2个全局buf，用于缓存ADC和DAC的数据，注意单位
uint32_t AudioADC1Buf[1024] = {0}; // 1024 * 4 = 4K
uint32_t AudioDACBuf[4096] = {0};  // 1024 * 4 = 4K

void audio_init(uint32_t SampleRate)
{
#ifdef CFG_AUDIO_WIDTH_24BIT
	uint32_t DACBitWidth = 24;
	AUDIO_BitWidth ADCBitWidth = ADC_WIDTH_24BITS;
#else
	uint32_t DACBitWidth = 16;
	AUDIO_BitWidth ADCBitWidth = ADC_WIDTH_16BITS;
#endif
	DACParamCt ct;
	ct.DACModel = DAC_Single;			//差分
	ct.DACLoadStatus = DAC_NOLoad;
	ct.PVDDModel = PVDD33;
	ct.DACEnergyModel = DACCommonEnergy;

	AudioADC_AnaInit(ADC1_MODULE, CHANNEL_LEFT, MIC_LEFT, Diff, ADCCommonEnergy, 22);
	AudioADC_DigitalInit(ADC1_MODULE, SampleRate, ADCBitWidth, (void *)AudioADC1Buf, sizeof(AudioADC1Buf));

	// DAC init
	AudioDAC_Init(&ct, SampleRate, DACBitWidth, (void *)AudioDACBuf, sizeof(AudioDACBuf), NULL, 0);
}




