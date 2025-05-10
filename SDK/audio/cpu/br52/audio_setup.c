/*
 ******************************************************************
 *					      Audio Setup
 *
 * Discription: 音频模块初始化，配置，调试等
 *
 * Notes:
 ******************************************************************
 */
#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "sdk_config.h"
#include "audio_adc.h"
#include "media/audio_energy_detect.h"
#include "adc_file.h"
#include "linein_file.h"
#include "asm/audio_common.h"
#include "gpio_config.h"
#include "audio_demo/audio_demo.h"
#include "gpadc.h"
#include "update.h"
#include "media/audio_general.h"
#include "media/audio_event_manager.h"
#include "asm/hw_eq.h"

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
#include "audio_dvol.h"
#endif

#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif

#if TCFG_AUDIO_DUT_ENABLE
#include "audio_dut_control.h"
#endif/*TCFG_AUDIO_DUT_ENABLE*/

#if TCFG_SMART_VOICE_ENABLE
#include "smart_voice.h"
#endif /*TCFG_SMART_VOICE_ENABLE*/

struct audio_dac_hdl dac_hdl;
struct audio_adc_hdl adc_hdl;

typedef struct {
    u8 audio_inited;
    atomic_t ref;
} audio_setup_t;
audio_setup_t audio_setup = {0};
#define __this      (&audio_setup)

extern struct dac_platform_data dac_data;

#ifndef TCFG_DAC_POWER_MODE
#define TCFG_DAC_POWER_MODE DAC_MODE_STANDARD_POWER
#endif

int get_dac_channel_num(void)
{
    return dac_hdl.channel;
}

static u8 audio_dac_hpvdd_check()
{
#ifndef CONFIG_CPU_BR52
    u8 res;
    u16 hpvdd = adc_get_voltage_blocking(AD_CH_AUDIO_HPVDD);
    /* printf("HPVDD: %d\n", hpvdd); */
    if (hpvdd > 1500) {
        res = DAC_HPVDD_18V;
        puts("DAC_HPVDD: 1.8V");
    } else {
        res = DAC_HPVDD_12V;
        puts("DAC_HPVDD: 1.2V");
    }
    SFR(JL_ADDA->ADDA_CON0,  0,  1,  0);					// AUDIO到SARADC的总测试通道使能
    SFR(JL_ADDA->ADDA_CON0,  3,  1,  0);					// DAC测试通道总使能
    SFR(JL_ADDA->ADDA_CON0,  4,  3,  0);					// DAC待测试信号选择位
    return res;
#else
    return 0;
#endif
}

/*
 * DAC开关状态回调，需要的时候，"#if 0" 改成 "#if 1"
 */
#if 0
void audio_dac_power_state(u8 state)
{
    switch (state) {
    case DAC_ANALOG_OPEN_PREPARE:
        //DAC打开前，即准备打开
        printf("DAC power_on start\n");
        break;
    case DAC_ANALOG_OPEN_FINISH:
        //DAC打开后，即打开完成
        printf("DAC power_on end\n");
        break;
    case DAC_ANALOG_CLOSE_PREPARE:
        //DAC关闭前，即准备关闭
        printf("DAC power_off start\n");
        break;
    case DAC_ANALOG_CLOSE_FINISH:
        //DAC关闭后，即关闭完成
        printf("DAC power_off end\n");
        break;
    }
}
#endif

void audio_dac_initcall(void)
{
    printf("audio_dac_initcall\n");

    audio_common_param_t common_param = {0};
    common_param.cic.en = 1;
    common_param.cic.scale = 0;
    if (dac_data.pa_sel) {
        if (dac_data.epa_dsm_mode == EPA_DSM_MODE_375K) {
            common_param.epa_clk_div = 7;
            common_param.cic.shift = 15;
        } else if (dac_data.epa_dsm_mode == EPA_DSM_MODE_750K) {
            common_param.epa_clk_div = 3;
            common_param.cic.shift = 11;
        } else if (dac_data.epa_dsm_mode == EPA_DSM_MODE_1500K) {
            common_param.epa_clk_div = 1;
            common_param.cic.shift = 7;
        } else {
            printf("[ERROR]EPA_DSM_MODE: %d !!!!!\n", dac_data.epa_dsm_mode);
        }
    } else {
        common_param.epa_clk_div = 0;
        common_param.fb_clk_div = 7;
        common_param.cic.shift = 15;
    }
    common_param.drc.bypass = 1;
    common_param.drc.threshold = 1023;
    common_param.drc.ratio = 64;
    common_param.drc.kneewidth = 16;
    common_param.drc.makeup_gain = 0;
    common_param.drc.attack_time = 3;
    common_param.drc.release_time = 10;

#if TCFG_AUDIO_DAC_VOLUME_BOOST
    dac_data.power_mode = DAC_MODE_LARGE_POWER;
    common_param.vcm_level = AUDIO_COMMON_VCM_LEVEL2;
#else
    dac_data.power_mode = DAC_MODE_STANDARD_POWER;
    common_param.vcm_level = AUDIO_COMMON_VCM_LEVEL1;
#endif
    //audio vbg trim配置

    common_param.vbg_i_trim_value = (JL_ADDA->ADDA_CON0 >> 20) & 0xf;  //默认VBG电流档位(没有trim过才会使用)
    if (!common_param.vbg_i_trim_value) {
        common_param.vbg_i_trim_value = 8;
    }
    common_param.pmu_vbg_value = efuse_get_audio_vbg_trim();
    if (common_param.pmu_vbg_value == 0x1F) {
        common_param.pmu_vbg_value = 11;
        printf("[Warning]audio vbg trim value invalid,default=%d\n", common_param.pmu_vbg_value);
    }
    int len = audio_event_notify(AUDIO_LIB_EVENT_VBG_TRIM_READ, (void *)&common_param.audio_vbg_value, sizeof(unsigned char));
    if (len != sizeof(unsigned char)) {
        common_param.audio_vbg_value = audio_common_vbg_trim(common_param.vcm_level, common_param.vbg_i_trim_value);
    }
    /* common_param.clock_mode = AUDIO_COMMON_CLK_DIG_SINGLE; */
    common_param.clock_mode = AUDIO_COMMON_CLK_DIF_XOSC;
    /* common_param.clock_mode = AUDIO_COMMON_CLK_DIG_XOSC; //低功耗配置时钟 */
    common_param.pa_sel = dac_data.pa_sel;
    common_param.vcm_cap_en = TCFG_AUDIO_VCM_CAP_EN;
    audio_common_init(&common_param);

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    audio_digital_vol_init(NULL, 0);
#endif
    dac_data.epa_clk_sel = clk_get("bt_pll") / 1000000;
    dac_data.max_sample_rate    = AUDIO_DAC_MAX_SAMPLE_RATE;
    dac_data.hpvdd_sel = audio_dac_hpvdd_check();
    dac_data.bit_width = audio_general_out_dev_bit_width();
    dac_data.mute_delay_isel = 2;
    dac_data.mute_delay_time = 15;
    audio_dac_init(&dac_hdl, &dac_data);
    //dac_hdl.ng.threshold = 4;			//DAC底噪优化阈值
    //dac_hdl.ng.detect_interval = 200;	//DAC底噪优化检测间隔ms

    //ANC & DAC_CIC时钟分配参数设置在audio_common_init & audio_dac_init之后
#if TCFG_AUDIO_ANC_ENABLE
    /*
       1、根据ANC参数生成CLOCK DIV以及ANC & DAC CIC配置
       2、读取ANC DAC DRC配置
    */
    audio_anc_common_param_init();
#endif/*TCFG_AUDIO_ANC_ENABLE*/

    audio_dac_set_analog_vol(&dac_hdl, 0);

#if AUD_DAC_TRIM_ENABLE
    if (dac_data.pa_sel == 0) {
        struct audio_dac_trim dac_trim = {0};
        int dac_trim_len = syscfg_read(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(dac_trim));
        if (dac_trim_len != sizeof(dac_trim)) {
            struct trim_init_param_t trim_init = {0};
            trim_init.precision = 1; //DAC trim的收敛精度(-precision, +precision)
            int ret = audio_dac_do_trim(&dac_hdl, &dac_trim, &trim_init);
            if ((ret == 0) && (__builtin_abs(dac_trim.left) < 50) && (__builtin_abs(dac_trim.right) < 50)) {
                /* puts("dac_trim_succ"); */
                syscfg_write(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(struct audio_dac_trim));
            } else {
                dac_trim.left = 0;
                dac_trim.right = 0;
            }
            audio_dac_close(&dac_hdl);
        }
        audio_dac_set_trim_value(&dac_hdl, &dac_trim);
    }
#endif

    audio_dac_set_fade_handler(&dac_hdl, NULL, audio_fade_in_fade_out);

    /*硬件SRC模块滤波器buffer设置，可根据最大使用数量设置整体buffer*/
    /* audio_src_base_filt_init(audio_src_hw_filt, sizeof(audio_src_hw_filt)); */

#if AUDIO_OUTPUT_AUTOMUTE
    mix_out_automute_open();
#endif  //#if AUDIO_OUTPUT_AUTOMUTE
}

static u8 audio_init_complete()
{
    if (!__this->audio_inited) {
        return 0;
    }
    return 1;
}

REGISTER_LP_TARGET(audio_init_lp_target) = {
    .name = "audio_init",
    .is_idle = audio_init_complete,
};

extern void dac_analog_power_control(u8 en);
void audio_fast_mode_test()
{
    audio_dac_set_volume(&dac_hdl, app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
    /* dac_analog_power_control(1);////将关闭基带，不开可发现，不可连接 */
    audio_dac_start(&dac_hdl);
    audio_adc_mic_demo_open(AUDIO_ADC_MIC_CH, 10, 16000, 1);

}

struct audio_adc_private_param adc_private_param = {
    .performance_mode = TCFG_ADC_PERFORMANCE_MODE,
    .mic_ldo_vsel   = TCFG_AUDIO_MIC_LDO_VSEL,
    /* .mic_ldo_isel   = TCFG_AUDIO_MIC_LDO_ISEL, */
    .adca_reserved0 = 0,
    .adcb_reserved0 = 0,
    .lowpower_lvl = 0,
};

#if TCFG_AUDIO_ADC_ENABLE
const struct adc_platform_cfg adc_platform_cfg_table[AUDIO_ADC_MAX_NUM] = {
#if TCFG_ADC0_ENABLE
    [0] = {
        .mic_mode           = TCFG_ADC0_MODE,
        .mic_ain_sel        = TCFG_ADC0_AIN_SEL,
        .mic_bias_sel       = TCFG_ADC0_BIAS_SEL,
        .mic_bias_rsel      = TCFG_ADC0_BIAS_RSEL,
        .power_io           = TCFG_ADC0_POWER_IO,
        .mic_dcc_en         = TCFG_ADC0_DCC_EN,
        .mic_dcc            = TCFG_ADC0_DCC_LEVEL,
    },
#endif
#if TCFG_ADC1_ENABLE
    [1] = {
        .mic_mode           = TCFG_ADC1_MODE,
        .mic_ain_sel        = TCFG_ADC1_AIN_SEL,
        .mic_bias_sel       = TCFG_ADC1_BIAS_SEL,
        .mic_bias_rsel      = TCFG_ADC1_BIAS_RSEL,
        .power_io           = TCFG_ADC1_POWER_IO,
        .mic_dcc_en         = TCFG_ADC1_DCC_EN,
        .mic_dcc            = TCFG_ADC1_DCC_LEVEL,
    },
#endif
#if TCFG_ADC2_ENABLE
    [2] = {
        .mic_mode           = TCFG_ADC2_MODE,
        .mic_ain_sel        = TCFG_ADC2_AIN_SEL,
        .mic_bias_sel       = TCFG_ADC2_BIAS_SEL,
        .mic_bias_rsel      = TCFG_ADC2_BIAS_RSEL,
        .power_io           = TCFG_ADC2_POWER_IO,
        .mic_dcc_en         = TCFG_ADC2_DCC_EN,
        .mic_dcc            = TCFG_ADC2_DCC_LEVEL,
    },
#endif
};
#endif

void audio_input_initcall(void)
{
    printf("audio_input_initcall\n");
#if TCFG_MC_BIAS_AUTO_ADJUST
    /* extern u8 mic_ldo_vsel_use_save; */
    /* extern u8 save_mic_ldo_vsel; */
    if (mic_ldo_vsel_use_save) {
        adc_private_param.mic_ldo_vsel = save_mic_ldo_vsel;
    }
#endif

    u16 dvol_441k = (u16)(50 * eq_db2mag(TCFG_ADC_DIGITAL_GAIN));
    u16 dvol_48k  = (u16)(35 * eq_db2mag(TCFG_ADC_DIGITAL_GAIN));
    adc_private_param.dvol_441k = (dvol_441k >= AUDIO_ADC_DVOL_LIMIT) ? AUDIO_ADC_DVOL_LIMIT : dvol_441k;
    adc_private_param.dvol_48k = (dvol_48k >= AUDIO_ADC_DVOL_LIMIT) ? AUDIO_ADC_DVOL_LIMIT : dvol_48k;
    audio_adc_init(&adc_hdl, &adc_private_param);
    /* adc_hdl.bit_width = audio_general_in_dev_bit_width(); */
    audio_adc_file_init();

#if TCFG_AUDIO_DUT_ENABLE
    audio_dut_init();
#endif /*TCFG_AUDIO_DUT_ENABLE*/

#if TCFG_AUDIO_LINEIN_ENABLE
    audio_linein_file_init();
#endif/*TCFG_AUDIO_LINEIN_ENABLE*/
}

#ifndef TCFG_AUDIO_DAC_LDO_VOLT_HIGH
#define TCFG_AUDIO_DAC_LDO_VOLT_HIGH 0
#endif

struct dac_platform_data dac_data = {//临时处理
    .dma_buf_time_ms = TCFG_AUDIO_DAC_BUFFER_TIME_MS,
    .performance_mode = TCFG_DAC_PERFORMANCE_MODE,
    .power_mode     = TCFG_DAC_POWER_MODE,
    .l_ana_gain     = TCFG_AUDIO_L_CHANNEL_GAIN,
    .r_ana_gain     = TCFG_AUDIO_R_CHANNEL_GAIN,
    .dcc_level      = 14,
    .bit_width      = DAC_BIT_WIDTH_16,
    .fade_en        = 1,
    .fade_points    = 1,
    .fade_volume    = 4,
    .pa_sel         = TCFG_AUDIO_DAC_PA_MODE,
    .epa_dsm_mode   = EPA_DSM_MODE_750K,
    .epa_pwm_mode   = EPA_PWM_MODE1,
    .ldo_volt       = TCFG_AUDIO_DAC_LDO_VOLT,
#if (TCFG_DAC_PERFORMANCE_MODE == DAC_MODE_HIGH_PERFORMANCE)
    .pa_isel0       = TCFG_AUDIO_DAC_HP_PA_ISEL0,
    .pa_isel1       = TCFG_AUDIO_DAC_HP_PA_ISEL1,
#else
    .pa_isel0       = TCFG_AUDIO_DAC_LP_PA_ISEL0,
    .pa_isel1       = TCFG_AUDIO_DAC_LP_PA_ISEL1,
#endif
};

static void wl_audio_clk_on(void)
{
    JL_WL_AUD->CON0 = 1;
    /*JL_ASS->CLK_CON |= BIT(0);//audio时钟*/
}

static int audio_init()
{
    wl_audio_clk_on();

    audio_general_init();

    audio_input_initcall();

#if TCFG_AUDIO_ANC_ENABLE
    anc_init();
#endif
    //AC700N DAC初始化在ANC之后，因为需要读取ANC 采样率以及DAC DRC配置
    audio_dac_initcall();

#if TCFG_SMART_VOICE_ENABLE
    audio_smart_voice_detect_init(NULL);
#endif /* #if TCFG_SMART_VOICE_ENABLE */
    __this->audio_inited = 1;
    return 0;
}
platform_initcall(audio_init);

static void audio_uninit()
{
    dac_power_off();
}
platform_uninitcall(audio_uninit);

/*关闭audio相关模块使能*/
static void audio_disable_all(void)
{
    printf("audio_disable_all\n");
    //DAC:DACEN
    JL_AUD->AUD_CON0 &= ~BIT(2);
    //ADC:ADCEN
    JL_AUD->AUD_CON0 &= ~(BIT(1) | BIT(4) | BIT(9));
    //EQ:
    JL_EQ->CON0 &= ~BIT(1);
    //FFT:
    //JL_FFT->CON = BIT(1);//置1强制关闭模块，不管是否已经运算完成
    //SRC:
    JL_SRC0->CON1 |= BIT(22);
    JL_SRC1->CON0 |= BIT(10);

    //ANC:anc_en anc_start
#if 0//build
    JL_ANC->CON0 &= ~(BIT(1) | BIT(29));
#endif

}

REGISTER_UPDATE_TARGET(audio_update_target) = {
    .name = "audio",
    .driver_close = audio_disable_all,
};

void dac_power_on(void)
{
    /* log_info(">>>dac_power_on:%d", __this->ref.counter); */
    if (atomic_inc_return(&__this->ref) == 1) {
        audio_dac_open(&dac_hdl);
    }
}

void dac_power_off(void)
{
    /*log_info(">>>dac_power_off:%d", __this->ref.counter);*/
    if (atomic_read(&__this->ref) != 0 && atomic_dec_return(&__this->ref)) {
        return;
    }
    audio_dac_close(&dac_hdl);
}

#define TRIM_VALUE_LR_ERR_MAX           (600)   // 距离参考值的差值限制
#define abs(x) ((x)>0?(x):-(x))
int audio_dac_trim_value_check(struct audio_dac_trim *dac_trim)
{
    s16 reference = 0;
    printf("audio_dac_trim_value_check %d %d\n", dac_trim->left, dac_trim->right);

#if TCFG_AUDIO_DAC_CONNECT_MODE != DAC_OUTPUT_MONO_R
    if (abs(dac_trim->left - reference) > TRIM_VALUE_LR_ERR_MAX) {
        printf("dac trim channel l err\n");
        return -1;
    }
#endif
#if TCFG_AUDIO_DAC_CONNECT_MODE != DAC_OUTPUT_MONO_L
    if (abs(dac_trim->right - reference) > TRIM_VALUE_LR_ERR_MAX) {
        printf("dac trim channel r err\n");
        return -1;
    }
#endif

    return 0;
}

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

