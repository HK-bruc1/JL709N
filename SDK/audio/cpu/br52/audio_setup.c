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
 * DAC MUTE/UNMUTE 回调
 */
#if 0
void audio_dac_ch_mute_notify(u8 mute_state, u8 step)
{
}
#endif

static void audio_common_initcall()
{
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
    common_param.aud_en = 1;
    audio_common_clock_open();
}

void audio_dac_initcall(void)
{
    printf("audio_dac_initcall\n");

    dac_data.epa_clk_sel = clk_get("bt_pll") / 1000000;
    dac_data.max_sample_rate    = AUDIO_DAC_MAX_SAMPLE_RATE;
    dac_data.hpvdd_sel = audio_dac_hpvdd_check();
    dac_data.bit_width = audio_general_out_dev_bit_width();
    dac_data.mute_delay_isel = 2;
    dac_data.mute_delay_time = 15;
    if ((get_chip_version() >= 0x03) && (get_chip_version() <= 0x05)) {
        dac_data.performance_mode = DAC_MODE_HIGH_PERFORMANCE;
        dac_data.pa_isel0 = (TCFG_AUDIO_DAC_HP_PA_ISEL0 < 5) ? (5) : (TCFG_AUDIO_DAC_HP_PA_ISEL0);
        dac_data.pa_isel1 = (TCFG_AUDIO_DAC_HP_PA_ISEL1 < 3) ? (3) : (TCFG_AUDIO_DAC_HP_PA_ISEL1);
    }
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
                puts("DAC_trim_verify:Succ");
                syscfg_write(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(struct audio_dac_trim));
            } else {
                puts("[Error]DAC_trim_verify:Error!!!");
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

#if ANC_MIC_REUSE_ENABLE
const struct adc_platform_cfg adc_user_cfg = {
    .mic_mode           = 1,	//模式
    .mic_ain_sel        = 1,	//输入端口
    .mic_bias_sel       = 32,	//供电端口
    .mic_bias_rsel      = 3,	//上拉电阻位
    .power_io           = 0,	//IO供电选择
    .mic_dcc_en         = 1,	//DCC使能
    .mic_dcc            = 1,	//DCC截止频率
};

/*
   ANC 复用MIC 硬件配置切换
   param: user 0: 使用可视化工具-音频配置;  1 使用adc_user_cfg 配置
*/
void audio_adc_user_cfg_change(u8 user)
{
    printf("%s, user %d\n", __func__, user);
    if (user) {
        //MIC0 硬件切换到自定义配置
        audio_adc_platform_cfg_change(ANC_MIC_REUSE_NUM, &adc_user_cfg);
    } else {
        //MIC0 硬件切换到可视化配置
        audio_adc_platform_cfg_change(ANC_MIC_REUSE_NUM, &adc_platform_cfg_table[ANC_MIC_REUSE_NUM]);
    }
}
#endif

#ifndef TCFG_AUDIO_DAC_LDO_VOLT_HIGH
#define TCFG_AUDIO_DAC_LDO_VOLT_HIGH 0
#endif

struct dac_platform_data dac_data = {//临时处理
    .dma_buf_time_ms = TCFG_AUDIO_DAC_BUFFER_TIME_MS,
    .performance_mode = TCFG_DAC_PERFORMANCE_MODE,
    .power_mode     = TCFG_DAC_POWER_MODE,
    .l_ana_gain     = TCFG_AUDIO_L_CHANNEL_GAIN,
    .r_ana_gain     = TCFG_AUDIO_R_CHANNEL_GAIN,
    .dcc_level      = 15,
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

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    audio_digital_vol_init(NULL, 0);
#endif

#if (TCFG_DAC_NODE_ENABLE || TCFG_AUDIO_ADC_ENABLE)
    audio_common_initcall();
#endif
    //AC700N DAC初始化在ANC之后，因为需要读取ANC 采样率以及DAC DRC配置
#if TCFG_DAC_NODE_ENABLE
    audio_dac_initcall();
#endif

#if TCFG_SMART_VOICE_ENABLE
    audio_smart_voice_detect_init(NULL);
#endif /* #if TCFG_SMART_VOICE_ENABLE */
    __this->audio_inited = 1;
    return 0;
}
platform_initcall(audio_init);

static void audio_uninit()
{
#if TCFG_DAC_NODE_ENABLE
    audio_dac_close(&dac_hdl);
#endif
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

