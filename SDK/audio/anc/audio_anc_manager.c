
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_anc_common.data.bss")
#pragma data_seg(".audio_anc_common.data")
#pragma const_seg(".audio_anc_common.text.const")
#pragma code_seg(".audio_anc_common.text")
#endif

/*
 ****************************************************************
 *							AUDIO ANC MANAGER
 * File  : audio_anc_manager.c
 * By    :
 * Notes : 存放ANC策略管理
 *
 ****************************************************************
 */
#include "app_config.h"

#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#include "audio_anc_common_plug.h"

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
#include "icsd_adt.h"
#include "icsd_adt_app.h"
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE
#include "rt_anc_app.h"
#endif

#if 1
#define anc_log	printf
#else
#define anc_log (...)
#endif


#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
/*
   配置不同业务场景下支持ANC_EXT 算法功能
   1、ANC模式、IDLE场景默认全支持
   2、多场景共存时，以不支持的场景优先
   3、数组修改注意
   		1）scene 需与枚举 enum ICSD_ADT_SCENE 的 USER SCENE 对齐
   		2）算法  需与枚举 enum ICSD_ADT_MODE  对齐
*/
const u8 anc_ext_support_scene[6][10] = {
    //0-智能免摘 1-风噪检测 2-广域点击 3-NULL 4-RTANC 5-入耳检测 6-环境自适应 7-NULL 8-NULL 9-啸叫检测
    { 1,         1,         1,         0,     1,      1,         1,           0,     0,     1         }, //A2DP场景
    { 0,         0,         0,         0,     1,      0,         0,           0,     0,     1         }, //ESCO场景
    { 0,         0,         0,         0,     1,      0,         0,           0,     0,     1         }, //MIC混响场景
    { 0,         0,         0,         0,     0,      0,         0,           0,     0,     0         }, //提示音场景
    { 0,         0,         0,         0,     1,      0,         0,           0,     0,     0         }, //ANC_OFF场景
#if ANC_MULT_TRANS_FB_ENABLE /*通透+FB 才能支持RTANC*/
    { 1,         1,         1,         0,     1,      1,         1,           0,     0,     1         }, //通透场景
#else
    { 1,         1,         1,         0,     0,/*NO*/1,         1,           0,     0,     1         }, //通透场景
#endif
};


//ANC模式切换时，启动算法功能 - 后续可被模块单独开关替代
int audio_anc_app_adt_mode_init(u8 enable)
{
    u16 adt_mode = 0;
#if TCFG_AUDIO_ANC_WIND_NOISE_DET_ENABLE
    adt_mode |= ADT_WIND_NOISE_DET_MODE;
    anc_ext_algorithm_state_update(ANC_EXT_ALGO_WIND_DET, ANC_EXT_ALGO_STA_OPEN, 0);
#endif
#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE
    adt_mode |= ADT_REAL_TIME_ADAPTIVE_ANC_MODE;
    anc_ext_algorithm_state_update(ANC_EXT_ALGO_RTANC, ANC_EXT_ALGO_STA_OPEN, 0);
#endif
#if TCFG_AUDIO_ANC_ENV_ADAPTIVE_GAIN_ENABLE
    adt_mode |= ADT_ENV_NOISE_DET_MODE;
#endif
#if TCFG_AUDIO_ANC_HOWLING_DET_ENABLE
    adt_mode |= ADT_HOWLING_DET_MODE;
    anc_ext_algorithm_state_update(ANC_EXT_ALGO_SOFT_HOWL_DET, ANC_EXT_ALGO_STA_OPEN, 0);
#endif
#if TCFG_AUDIO_ANC_ADAPTIVE_CMP_EN
    anc_ext_adaptive_cmp_tool_en_set(1);
    anc_ext_algorithm_state_update(ANC_EXT_ALGO_ADAPTIVE_CMP, ANC_EXT_ALGO_STA_OPEN, 0);
#endif
#if TCFG_AUDIO_ADAPTIVE_EQ_ENABLE
    anc_ext_adaptive_eq_tool_en_set(1);
    anc_ext_algorithm_state_update(ANC_EXT_ALGO_ADAPTIVE_EQ, ANC_EXT_ALGO_STA_OPEN, 0);
#endif
    printf("audio_anc_app_adt_mode_init 0x%x, en %d\n", adt_mode, enable);
    icsd_adt_app_mode_set(adt_mode, enable);
    return 0;
}

#endif

//ANC事件策略管理
int audio_anc_event_notify(enum anc_event event, int arg)
{
    int ret = 0;
    switch (event) {
#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
    case ANC_EVENT_ADT_INIT:
        audio_anc_app_adt_mode_init(1);
        break;
#endif
    case ANC_EVENT_ADT_RESET:
#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE
        enum ICSD_ADT_SCENE scene = (enum ICSD_ADT_SCENE)arg;
        //通话场景复位ADT，需保留RTANC算法临时参数
        if ((scene & ADT_SCENE_ESCO) && audio_anc_real_time_adaptive_state_get()) {
            audio_rtanc_var_cache_set(1);
        }
#endif
        break;
    default:
        break;
    };
    return ret;
}


#endif
