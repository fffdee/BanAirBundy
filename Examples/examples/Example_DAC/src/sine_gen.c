/**
 *************************************************************************************
 * @file sine_gen.c
 * @brief 
 *
 * @author castle 
 * @version v1.1.2
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#include "type.h"
#include <math.h>
#include <nds32_intrinsic.h>
#include <nds32_utils_math.h>
#include "sine_gen.h"

void du_efft_fadein_sw(int16_t* pcm_in, uint16_t pcm_length, uint16_t ch, uint16_t ch_ex);
void du_efft_fadeout_sw(int16_t* pcm_in, uint16_t pcm_length, uint16_t ch, uint16_t ch_ex);

SINE_GEN_ERROR_CODE sine_generator_config_channel(SineGenContext *sgc, int32_t channel_enable)
{
	if(sgc->channel_enable & 0x01 != channel_enable & 0x01)
	{
		sgc->left_phase = 0;
		if(channel_enable & 0x01)
			sgc->left_fio = SINE_GEN_STATUS_FADE_IN;
		else
			sgc->left_fio = SINE_GEN_STATUS_FADE_OUT;
	}

	if(sgc->channel_enable & 0x02 != channel_enable & 0x02)
	{
		sgc->right_phase = 0;
		if(channel_enable & 0x02)
			sgc->right_fio = SINE_GEN_STATUS_FADE_IN;
		else
			sgc->right_fio = SINE_GEN_STATUS_FADE_OUT;
	}

	sgc->channel_enable = channel_enable;

	return SINE_GEN_ERROR_OK;
}

SINE_GEN_ERROR_CODE sine24_generator_config_channel(Sine32GenContext *sgc, int32_t channel_enable)
{
	if(sgc->channel_enable & 0x01 != channel_enable & 0x01)
	{
		sgc->left_phase = 0;
		if(channel_enable & 0x01)
			sgc->left_fio = SINE_GEN_STATUS_FADE_IN;
		else
			sgc->left_fio = SINE_GEN_STATUS_FADE_OUT;
	}

	if(sgc->channel_enable & 0x02 != channel_enable & 0x02)
	{
		sgc->right_phase = 0;
		if(channel_enable & 0x02)
			sgc->right_fio = SINE_GEN_STATUS_FADE_IN;
		else
			sgc->right_fio = SINE_GEN_STATUS_FADE_OUT;
	}

	sgc->channel_enable = channel_enable;

	return SINE_GEN_ERROR_OK;
}

//16bit
SINE_GEN_ERROR_CODE sine_generator_config_freq(SineGenContext *sgc, int32_t left_freq, int32_t right_freq)
{

	if(sgc->left_freq != left_freq)
	{
		sgc->left_phase = 0;
		sgc->left_freq = left_freq;
		sgc->left_step = (int32_t)PCM_16_MAX * 2 * sgc->left_freq / sgc->sample_rate;
	}

	if(sgc->right_freq != right_freq)
	{
		sgc->right_phase = 0;
		sgc->right_freq = right_freq;
		sgc->right_step = (int32_t)PCM_16_MAX * 2 * sgc->right_freq / sgc->sample_rate;
	}

	return SINE_GEN_ERROR_OK;
}

SINE_GEN_ERROR_CODE sine_generator_config_ampl(SineGenContext *sgc, int32_t left_ampl, int32_t right_ampl)
{
	sgc->left_scale = (int32_t)roundf(pow(10.0f, left_ampl/200.0f) * PCM_16_MAX);
	sgc->right_scale = (int32_t)roundf(pow(10.0f, right_ampl/200.0f) * PCM_16_MAX);

	return SINE_GEN_ERROR_OK;
}

SINE_GEN_ERROR_CODE sine_generator_init(SineGenContext *sgc, int32_t sample_rate, int16_t channel_enable, int32_t left_freq, int32_t right_freq, int32_t left_ampl, int32_t right_ampl)
{
	bool ret;
	int i;

	if(left_freq > sample_rate/2 || right_freq > sample_rate/2)
		return SINE_GEN_FRQ_ERROR;

	if(left_ampl < -960 || left_ampl > 0)
		return SINE_GEN_AMPL_ERROR;

	if(right_ampl < -960 || right_ampl > 0)
		return SINE_GEN_AMPL_ERROR;

	memset(sgc, 0x00, sizeof(SineGenContext));
	sgc->sample_rate = sample_rate;
	sgc->channel_enable = channel_enable;
	sgc->left_freq = left_freq;
	sgc->right_freq = right_freq;
	sgc->left_step = (int32_t)PCM_16_MAX * 2 * sgc->left_freq / sgc->sample_rate;
	sgc->right_step = (int32_t)PCM_16_MAX * 2 * sgc->right_freq / sgc->sample_rate;

	sine_generator_config_ampl(sgc, left_ampl, right_ampl);

	sgc->left_phase = 0;
	sgc->right_phase = 0;

	sgc->left_fio = SINE_GEN_STATUS_NONE;
	sgc->right_fio = SINE_GEN_STATUS_NONE;

	return SINE_GEN_ERROR_OK;
}

SINE_GEN_ERROR_CODE sine_generator_apply(SineGenContext *sgc, int16_t *pcm_in, int16_t *pcm_out, uint16_t pcm_len)
{
	int32_t i;
	int16_t l_v, r_v;
	for(i = 0; i<pcm_len; i++)
	{
		if(sgc->channel_enable & 0x01)
		{
			l_v = ((nds32_sin_q15((int16_t)sgc->left_phase) * sgc->left_scale) + PCM_16_HALF) >> 15;
			pcm_out[i*2 + 0] = __nds32__clips(((int32_t)pcm_in[i*2 + 0] + l_v), (16)-1);
			sgc->left_phase += sgc->left_step;
		}
		
		if(sgc->channel_enable & 0x02)
		{
			r_v = ((nds32_sin_q15((int16_t)sgc->right_phase) * sgc->right_scale) + PCM_16_HALF) >> 15;
			pcm_out[i*2 + 1] = __nds32__clips(((int32_t)pcm_in[i*2 + 1] + r_v), (16)-1);
			sgc->right_phase += sgc->right_step;
		}
		
	}

	return SINE_GEN_ERROR_OK;
}

//32bits
SINE_GEN_ERROR_CODE sine32_generator_config_freq(Sine32GenContext *sgc, int32_t left_freq, int32_t right_freq)
{

	if(sgc->left_freq != left_freq)
	{
		sgc->left_phase = 0;
		sgc->left_freq = left_freq;
		sgc->left_step = (int64_t)PCM_32_MAX * 2 * sgc->left_freq / sgc->sample_rate;
	}

	if(sgc->right_freq != right_freq)
	{
		sgc->right_phase = 0;
		sgc->right_freq = right_freq;
		sgc->right_step = (int64_t)PCM_32_MAX * 2 * sgc->right_freq / sgc->sample_rate;
	}

	return SINE_GEN_ERROR_OK;
}

SINE_GEN_ERROR_CODE sine32_generator_config_ampl(Sine32GenContext *sgc, int32_t left_ampl, int32_t right_ampl)
{
	sgc->left_scale = (int64_t)roundf(pow(10.0f, left_ampl/200.0f) * PCM_32_MAX);
	sgc->right_scale = (int64_t)roundf(pow(10.0f, right_ampl/200.0f) * PCM_32_MAX);

	return SINE_GEN_ERROR_OK;
}

SINE_GEN_ERROR_CODE sine32_generator_init(Sine32GenContext *sgc, uint16_t sample_rate, int16_t channel_enable, int32_t left_freq, int32_t right_freq, int32_t left_ampl, int32_t right_ampl)
{
	bool ret;
	int i;

	if(left_freq > sample_rate/2 || right_freq > sample_rate/2)
		return SINE_GEN_FRQ_ERROR;

	if(left_ampl < -960 || left_ampl > 0)
		return SINE_GEN_AMPL_ERROR;

	if(right_ampl < -960 || right_ampl > 0)
		return SINE_GEN_AMPL_ERROR;

	memset(sgc, 0x00, sizeof(Sine32GenContext));
	sgc->sample_rate = sample_rate;
	sgc->channel_enable = channel_enable;
	sgc->left_freq = left_freq;
	sgc->right_freq = right_freq;
	sgc->left_step = (int64_t)PCM_32_MAX * 2 * sgc->left_freq / sgc->sample_rate;
	sgc->right_step = (int64_t)PCM_32_MAX * 2 * sgc->right_freq / sgc->sample_rate;

	sine32_generator_config_ampl(sgc, left_ampl, right_ampl);

	sgc->left_phase = 0;
	sgc->right_phase = 0;

	sgc->left_fio = SINE_GEN_STATUS_NONE;
	sgc->right_fio = SINE_GEN_STATUS_NONE;

	return SINE_GEN_ERROR_OK;
}

SINE_GEN_ERROR_CODE sine32_generator_apply(Sine32GenContext *sgc, int32_t *pcm_in, int32_t *pcm_out, uint16_t pcm_len)
{
	int32_t i;
	int32_t l_v, r_v;
	for(i = 0; i<pcm_len; i++)
	{
		if(sgc->channel_enable & 0x01)
		{
			l_v = ((nds32_sin_q31((int32_t)sgc->left_phase) * sgc->left_scale) + PCM_32_HALF) >> 31;
			pcm_out[i*2 + 0] = __nds32__clips(((int64_t)pcm_in[i*2 + 0] + l_v), (32)-1);
			sgc->left_phase += sgc->left_step;
		}
		
		if(sgc->channel_enable & 0x02)
		{
			r_v = ((nds32_sin_q31((int32_t)sgc->right_phase) * sgc->right_scale) + PCM_32_HALF) >> 31;
			pcm_out[i*2 + 1] = __nds32__clips(((int64_t)pcm_in[i*2 + 1] + r_v), (32)-1);
			sgc->right_phase += sgc->right_step;
		}
	}

	return SINE_GEN_ERROR_OK;
}

//24bits
SINE_GEN_ERROR_CODE sine24_generator_config_freq(Sine32GenContext *sgc, int32_t left_freq, int32_t right_freq)
{

	if(sgc->left_freq != left_freq)
	{
		sgc->left_phase = 0;
		sgc->left_freq = left_freq;
		sgc->left_step = (int64_t)PCM_32_MAX * 2 * sgc->left_freq / sgc->sample_rate;
	}

	if(sgc->right_freq != right_freq)
	{
		sgc->right_phase = 0;
		sgc->right_freq = right_freq;
		sgc->right_step = (int64_t)PCM_32_MAX * 2 * sgc->right_freq / sgc->sample_rate;
	}

	return SINE_GEN_ERROR_OK;
}

SINE_GEN_ERROR_CODE sine24_generator_config_ampl(Sine32GenContext *sgc, int32_t left_ampl, int32_t right_ampl)
{
	sgc->left_scale = (int64_t)roundf(pow(10.0f, left_ampl/200.0f) * PCM_24_MAX);
	sgc->right_scale = (int64_t)roundf(pow(10.0f, right_ampl/200.0f) * PCM_24_MAX);

	return SINE_GEN_ERROR_OK;
}

SINE_GEN_ERROR_CODE sine24_generator_init(Sine32GenContext *sgc, int32_t sample_rate, int16_t channel_enable, int32_t left_freq, int32_t right_freq, int32_t left_ampl, int32_t right_ampl)
{
	bool ret;
	int i;

	if(left_freq > sample_rate/2 || right_freq > sample_rate/2)
		return SINE_GEN_FRQ_ERROR;

	if(left_ampl < -960 || left_ampl > 0)
		return SINE_GEN_AMPL_ERROR;

	if(right_ampl < -960 || right_ampl > 0)
		return SINE_GEN_AMPL_ERROR;

	memset(sgc, 0x00, sizeof(Sine32GenContext));
	sgc->sample_rate = sample_rate;
	sgc->channel_enable = channel_enable;
	sgc->left_freq = left_freq;
	sgc->right_freq = right_freq;
	sgc->left_step = (int64_t)PCM_32_MAX * 2 * sgc->left_freq / sgc->sample_rate;
	sgc->right_step = (int64_t)PCM_32_MAX * 2 * sgc->right_freq / sgc->sample_rate;

	sine24_generator_config_ampl(sgc, left_ampl, right_ampl);

	sgc->left_phase = 0;
	sgc->right_phase = 0;

	sgc->left_fio = SINE_GEN_STATUS_NONE;
	sgc->right_fio = SINE_GEN_STATUS_NONE;

	return SINE_GEN_ERROR_OK;
}

SINE_GEN_ERROR_CODE sine24_generator_apply(Sine32GenContext *sgc, int32_t *pcm_in, int32_t *pcm_out, uint16_t pcm_len)
{
	int32_t i;
	int32_t l_v, r_v;
	for(i = 0; i<pcm_len; i++)
	{
		if(sgc->channel_enable & 0x01)
		{
			l_v = (((nds32_sin_q31((int32_t)sgc->left_phase) >> 8) * sgc->left_scale) + PCM_24_HALF) >> 23;
			pcm_out[i*2 + 0] = __nds32__clips(((int64_t)pcm_in[i*2 + 0] + l_v), (24)-1);
			sgc->left_phase += sgc->left_step;
		}

		if(sgc->channel_enable & 0x02)
		{
			r_v = (((nds32_sin_q31((int32_t)sgc->right_phase) >> 8) * sgc->right_scale) + PCM_24_HALF) >> 23;
			pcm_out[i*2 + 1] = __nds32__clips(((int64_t)pcm_in[i*2 + 1] + r_v), (24)-1);
			sgc->right_phase += sgc->right_step;
		}
	}

	return SINE_GEN_ERROR_OK;
}

