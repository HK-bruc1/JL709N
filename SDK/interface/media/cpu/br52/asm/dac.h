#ifndef __CPU_DAC_H__
#define __CPU_DAC_H__


#include "generic/typedef.h"
#include "generic/atomic.h"
#include "os/os_api.h"
#include "audio_src.h"
#include "system/spinlock.h"
#include "audio_def.h"
#include "audio_general.h"

/***************************************************************************
  							Audio DAC Features
Notes:以下为芯片规格定义，不可修改，仅供引用
***************************************************************************/
#define AUDIO_DAC_CHANNEL_NUM	2	//DAC通道数

#define DACVDD_LDO_1_20V        0
#define DACVDD_LDO_1_25V        1
#define DACVDD_LDO_1_30V        2
#define DACVDD_LDO_1_35V        3

#define AUDIO_DAC_SYNC_IDLE             0
#define AUDIO_DAC_SYNC_START            1
#define AUDIO_DAC_SYNC_NO_DATA          2
#define AUDIO_DAC_SYNC_ALIGN_COMPLETE   3

#define AUDIO_SRC_SYNC_ENABLE   1
#define SYNC_LOCATION_FLOAT      1
#if SYNC_LOCATION_FLOAT
#define PCM_PHASE_BIT           0
#else
#define PCM_PHASE_BIT           8
#endif

#define DA_LEFT        0
#define DA_RIGHT       1

/************************************
             dac性能模式
************************************/
#define DAC_TRIM_SEL_RDAC_LP            0
#define DAC_TRIM_SEL_RDAC_LN            0
#define DAC_TRIM_SEL_RDAC_RP            1
#define DAC_TRIM_SEL_RDAC_RN            1
#define DAC_TRIM_SEL_PA_LP              2
#define DAC_TRIM_SEL_PA_LN              2
#define DAC_TRIM_SEL_PA_RP              3
#define DAC_TRIM_SEL_PA_RN              3
#define DAC_TRIM_SEL_VCM_L              4
#define DAC_TRIM_SEL_VCM_R              5

#define DAC_TRIM_CH_L                  0
#define DAC_TRIM_CH_R                  1

/************************************
             dac性能模式
************************************/
// TCFG_DAC_PERFORMANCE_MODE
#define	DAC_MODE_HIGH_PERFORMANCE          (0)
#define	DAC_MODE_LOW_POWER		           (1)

/************************************
             dac性能模式
************************************/
// TCFG_DAC_POWER_MODE
#define	DAC_MODE_STANDARD_POWER	           (0)
#define	DAC_MODE_LARGE_POWER               (1)
#define	DAC_MODE_ULTRA_POWER               (2)

/************************************
             hpvdd档位
************************************/
#define DAC_HPVDD_18V              (0)
#define DAC_HPVDD_12V              (1)


#define DAC_ANALOG_OPEN_PREPARE         (1) //DAC打开前，即准备打开
#define DAC_ANALOG_OPEN_FINISH          (2)	//DAC打开后，即打开完成
#define DAC_ANALOG_CLOSE_PREPARE        (3) //DAC关闭前，即准备关闭
#define DAC_ANALOG_CLOSE_FINISH         (4) //DAC关闭后，即关闭完成
//在应用层重定义 audio_dac_power_state 函数可以获取dac模拟开关的状态
//void audio_dac_power_state(u8 state){}

struct audio_dac_hdl;
struct dac_platform_data {
    u8 vcm_cap_en;      //配1代表走外部通路,vcm上有电容时,可以提升电路抑制电源噪声能力，提高ADC的性能，配0相当于vcm上无电容，抑制电源噪声能力下降,ADC性能下降
    u8 power_on_mode;
    u8 performance_mode;
    u8 power_mode;              //功率模式
    u8 hpvdd_sel;
    u8 l_ana_gain;
    u8 r_ana_gain;
    u8 dcc_level;
    u8 bit_width;             	// DAC输出位宽
    u8 fade_en;                 //硬件数字音量淡入淡出使能
    u8 fade_points;             //淡入淡出点数档位
    u8 fade_volume;             //淡入淡出音量档位
    u8 pa_sel;                  //输出级选择 0:DACPA 1:EPA
    u8 epa_clk_sel;
    u8 epa_dsm_mode;
    u8 epa_pwm_mode;
    u8 ldo_volt;                //电压：0~3
    u8 pa_isel0;                //电流：3~5
    u8 pa_isel1;                //电流：2~6
    u8 mute_delay_time;         //开关机延时参数
    u8 mute_delay_isel;         //开关机速度参数
    u16 dma_buf_time_ms;    // DAC dma buf 大小
    s16 *dig_vol_tab;
    u32 digital_gain_limit;
    u32 max_sample_rate;    	// 支持的最大采样率
};


struct trim_init_param_t {
    u8 clock_mode;
    u8 power_level;
    s16 precision;
    struct audio_dac_trim *dac_trim;
    struct audio_dac_hdl *dac;              /* DAC设备*/
};

struct audio_dac_trim {
    s16 left;
    s16 right;
    s16 vcomo;
};

// *INDENT-OFF*
struct audio_dac_sync {
    u8 channel;
    u8 start;
    u8 fast_align;
    int fast_points;
    u32 input_num;
    int phase_sub;
    int in_rate;
    int out_rate;
#if AUDIO_SRC_SYNC_ENABLE
    struct audio_src_sync_handle *src_sync;
    void *buf;
    int buf_len;
    void *filt_buf;
    int filt_len;
#else
    struct audio_src_base_handle *src_base;
#endif
#if SYNC_LOCATION_FLOAT
    double pcm_position;
#else
    u32 pcm_position;
#endif
    void *priv;
    void (*handler)(void *priv, u8 state);
};


struct audio_dac_hdl {
    u8 analog_inited;
    u8 digital_inited;
    volatile u8 state;
    u8 ng_threshold;
    u8 gain;
    u8 channel;
    u8 power_on;
    u8 need_close;
    u8 mute_ch;               //DAC PA Mute Channel
    u8 mute_step;             //DAC MUTE当前步骤
    u8 mute_state;            //DAC MUTE当前状态
    u8 mute_update;           //DAC MUTE最新状态
	u8 dvol_mute;             //DAC数字音量是否mute
    u8 dac_read_flag;	//AEC可读取DAC参考数据的标志
    u8 anc_dac_open;
    u8 protect_fadein;
    u8 vol_set_en;
    u8 dec_channel_num;
    u16 d_volume[2];
    u16 start_ms;
    u16 delay_ms;
    u16 start_points;
    u16 delay_points;
    u16 prepare_points;//未开始让DAC真正跑之前写入的PCM点数
    u16 irq_points;
    s16 protect_time;
    s16 protect_pns;
    s16 fadein_frames;
    s16 fade_vol;

    u16 unread_samples;             /*未读样点个数*/
    u16 mute_timer;           //DAC PA Mute Timer
    s16 *output_buf;
    u32 sample_rate;
    u32 digital_gain_limit;
    u32 output_buf_len;
    OS_SEM *sem;
    OS_MUTEX mutex;
    spinlock_t lock;
    const struct dac_platform_data *pd;
	struct audio_dac_noisegate ng;
    void (*fade_handler)(u8 left_gain, u8 right_gain);
};

enum DAC_PA_MODE{
	DAC_PA_MODE_DEFAULT,
	DAC_PA_MODE_CLASSD,
};


#endif

