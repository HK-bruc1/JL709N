#ifndef _AUDIO_COMMON_H_
#define _AUDIO_COMMON_H_

#include "generic/typedef.h"
#include "system/includes.h"

/************************************
             dac时钟
************************************/
#define	AUDIO_COMMON_CLK_DIG_SINGLE			(0)
#define	AUDIO_COMMON_CLK_DIF_XOSC			(1)
#define	AUDIO_COMMON_CLK_DIG_XOSC			(2) //低功耗配置时钟

#define	AUDIO_COMMON_VCM_LEVEL1		        (0) // VCM = 0.50V(适配DAC标准功率模式20mW)
#define	AUDIO_COMMON_VCM_LEVEL2		        (1) // VCM = 0.60V(适配DAC大功率模式30mW)
#define	AUDIO_COMMON_VCM_LEVEL3		        (2) // VCM = 0.66V(适配DAC超大功率模式50mW)

typedef struct {
    u8 en;				// DAC走ANC Sz通路(CIC)通路使能，一般存在ANC场景时默认要置1开启；若没有ANC场景，则可选择置0节省功耗
    u8 scale;			// DAC CIC增益细调
    u8 shift;			// DAC CIC增益粗调，19@CIC=187.5k, 15@CIC=375k, 11@CIC=750k !!!
} audio_dac_cic_t;

//DAC DRC结构
typedef struct {
    u16 threshold;         	// DAC drc threshold
    u8 ratio;            	// DAC drc ratio
    u8 makeup_gain;         // DAC drc makeup-gain
    u8 kneewidth;           // DAC drc kneewidth
    u8 attack_time;         // DAC drc attack time
    u8 release_time;        // DAC drc release time
    u8 bypass;				// DAC DRC bypass: 0@not bypass DRC, 1@bypass DRC
} audio_dac_drc_t;

typedef struct {
    u8 clock_mode;
    u8 vbg_v_trim_value0;
    u8 vbg_v_trim_value1;
    u8 vbg_i_trim_value;
    u8 vcm_cap_en;
    u8 vcm_level;
    u8 clk_mode;			// 音频时钟模式选择, 0: 数字单端时钟		1: 差分晶振时钟
    u8 pa_sel;
    u8 aud_en;				// ADDA总使能开关
    u8 lpadc_cken;			// LPADC时钟使能
    u8 plnk_cken;			// PLNK时钟使能
    u8 a2d_cken;			// AD2DA_HW时钟总使能

    u8 ff_clk_div;
    u8 fb_clk_div;
    u8 sz_clk_div;

    audio_dac_cic_t cic;
    audio_dac_drc_t drc;
} audio_common_param_t;

void audio_delay(int time_ms);
void audio_common_init(audio_common_param_t *param);
audio_common_param_t *audio_common_get_param(void);

void audio_common_audio_init(void *adc_data, void *dac_data);
void *audio_common_get_dac_data(void);
void *audio_common_get_adc_data(void);
void audio_common_set_power_level(u8 power_level);
void audio_common_clock_open(void);
void audio_common_clock_close(void);
void audio_common_power_open(void);
void audio_common_power_close(void);

void audio_common_dac_cic_set(audio_dac_cic_t *cic);
void audio_common_dac_cic_update(void);
void audio_common_dac_drc_set(audio_dac_drc_t *drc);
void audio_common_dac_drc_update(void);

void audio_common_cic_clk_div_set(u8 ff_div, u8 fb_div, u8 sz_div);
void audio_common_anc_clock_open(void);

int audio_adc_digital_status_add_check(int add);
int audio_adc_analog_status_add_check(u8 ch_index, int add);
int audio_dac_digital_status_add_check(int add);
int audio_dac_analog_status_add_check(int add);
u16 audio_hpvdd_hw_sel_check(void);
u8 audio_common_vbg_trim(u8 vcm_level, u8 vbg_i_value);

#endif // _AUDIO_COMMON_H_

