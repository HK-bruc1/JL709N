/*
 ******************************************************************************************
 *							Audio Config
 *
 * Discription: 音频模块与芯片系列相关配置
 *
 * Notes:
 ******************************************************************************************
 */
#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "audio_config.h"
#include "clock_manager/clock_manager.h"

/*
 *******************************************************************
 *						Audio Codec Config
 *******************************************************************
 */
const int config_audio_dac_mix_enable = 1;

//***********************
//*		AAC Codec       *
//***********************
const int AAC_DEC_MP4A_LATM_ANALYSIS = 1;
const int AAC_DEC_LIB_SUPPORT_24BIT_OUTPUT = 0;
const int WTS_DEC_LIB_SUPPORT_24BIT_OUTPUT = 1;

//***********************
//*		Audio Params    *
//***********************
void audio_adc_param_fill(struct mic_open_param *mic_param, struct adc_file_param *param)
{
    mic_param->mic_mode      = param->mic_mode;
    mic_param->mic_ain_sel   = param->mic_ain_sel;
    mic_param->mic_bias_sel  = param->mic_bias_sel;
    mic_param->mic_bias_rsel = param->mic_bias_rsel;
    mic_param->mic_dcc       = param->mic_dcc;
    mic_param->mic_dcc_en    = param->mic_dcc_en;
}

//***********************
//*		Audio Clock     *
//***********************
int aud_clock_alloc(const char *name, u32 clk)
{
    y_printf("aud_clock_alloc:%s(%d)\n", name, clk);
    return clock_alloc(name, clk);
}

int aud_clock_free(char *name)
{
    y_printf("aud_clock_alloc:%s\n", name);
    return clock_free(name);

}
