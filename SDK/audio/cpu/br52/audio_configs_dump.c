#include "generic/typedef.h"
#include "audio_dac.h"
#include "audio_adc.h"

/*音频模块寄存器跟踪*/
void audio_adda_dump(void) //打印所有的dac,adc寄存器
{
    printf("JL_WL_AUD CON0:%x", JL_WL_AUD->CON0);
    printf("DAC_CON0:%x", JL_AUDDAC->DAC_CON0);
    printf("DAC_CON1:%x", JL_AUDDAC->DAC_CON1);
    printf("DAC_CON2:%x", JL_AUDDAC->DAC_CON2);
    printf("DAC_CON3:%x", JL_AUDDAC->DAC_CON3);
    printf("ADC_CON0:%x", JL_AUDADC->ADC_CON0);
    printf("ADC_CON1:%x", JL_AUDADC->ADC_CON1);
    printf("ADC_CON2:%x", JL_AUDADC->ADC_CON2);
    printf("DAC_VL0:%x", JL_AUDDAC->DAC_VL0);
    printf("DAC_TM0:%x", JL_AUDDAC->DAC_TM0);
    printf("ADDA_CON0:%x    1:%x\n", JL_ADDA->ADDA_CON0, JL_ADDA->ADDA_CON1);
    printf("DAA_CON 0:%x 	1:%x,	2:%x,	7:%x\n", JL_ADDA->DAA_CON0, JL_ADDA->DAA_CON1, JL_ADDA->DAA_CON2, JL_ADDA->DAA_CON7);
    printf("ADA_CON 0:%x	1:%x	2:%x	3:%x	4:%x	5:%x	6:%x\n", JL_ADDA->ADA_CON0, JL_ADDA->ADA_CON1, JL_ADDA->ADA_CON2, JL_ADDA->ADA_CON3, JL_ADDA->ADA_CON4, JL_ADDA->ADA_CON5, JL_ADDA->ADA_CON6);
    printf("AUD_CON 0:%x	1:%x	2:%x	3:%x	4:%x	5:%x	6:%x	7:%x	8:%x\n", JL_AUD->AUD_CON0, JL_AUD->AUD_CON1, JL_AUD->AUD_CON2, JL_AUD->AUD_CON3, JL_AUD->AUD_CON4, JL_AUD->AUD_CON5, JL_AUD->AUD_CON6, JL_AUD->AUD_CON7, JL_AUD->AUD_CON8);
}

/*音频模块配置跟踪*/
void audio_config_dump()
{
    u8 dac_bit_width = ((JL_AUDDAC->DAC_CON0 & BIT(20)) ? 24 : 16);
    u8 adc_bit_width = ((JL_AUDADC->ADC_CON0 & BIT(20)) ? 24 : 16);
    int dac_dgain_max = 16384;
    int dac_again_max = 1;
    int mic_gain_max = 5;
    u8 dac_dcc = (JL_AUDDAC->DAC_CON0 >> 12) & 0xF;
    u8 mic0_dcc = (JL_AUDADC->ADC_CON1 >> 12) & 0xF;
    u8 mic1_dcc = (JL_AUDADC->ADC_CON1 >> 16) & 0xF;
    u8 mic2_dcc = (JL_AUDADC->ADC_CON1 >> 20) & 0xF;

    u8 dac_again_l = (JL_ADDA->DAA_CON1 >> 3) & 0x1;
    u8 dac_again_r = (JL_ADDA->DAA_CON2 >> 3) & 0x1;
    u32 dac_dgain_l = JL_AUDDAC->DAC_VL0 & 0xFFFF;
    u32 dac_dgain_r = (JL_AUDDAC->DAC_VL0 >> 16) & 0xFFFF;
    u8 mic0_0_6 = (JL_ADDA->ADA_CON1 >> 9) & 0x1;
    u8 mic1_0_6 = (JL_ADDA->ADA_CON2 >> 9) & 0x1;
    u8 mic2_0_6 = (JL_ADDA->ADA_CON3 >> 9) & 0x1;
    u8 mic0_gain = (JL_ADDA->ADA_CON1 >> 10) & 0x7;
    u8 mic1_gain = (JL_ADDA->ADA_CON2 >> 10) & 0x7;
    u8 mic2_gain = (JL_ADDA->ADA_CON3 >> 10) & 0x7;
    int dac_sr = audio_dac_get_sample_rate_base_reg();
    int adc_sr = audio_adc_mic_get_sample_rate();

    printf("[ADC]BitWidth:%d,DCC:%d,%d,%d,SR:%d\n", adc_bit_width, mic0_dcc, mic1_dcc, mic2_dcc, adc_sr);
    printf("[ADC]Gain(Max:%d):%d,%d,%d,6dB_Boost:%d,%d,%d,\n", mic_gain_max, mic0_gain, mic1_gain, mic2_gain, \
           mic0_0_6, mic1_0_6, mic2_0_6);

    printf("[DAC]BitWidth:%d,DCC:%d,SR:%d\n", dac_bit_width, dac_dcc, dac_sr);
    printf("[DAC]AGain(Max:%d):%d,%d,DGain(Max:%d):%d,%d\n", dac_again_max, dac_again_l, dac_again_r, \
           dac_dgain_max, dac_dgain_l, dac_dgain_r);
}

