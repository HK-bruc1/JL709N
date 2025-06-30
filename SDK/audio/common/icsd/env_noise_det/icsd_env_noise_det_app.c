#include "app_config.h"
#if ((defined TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN) && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN && \
	 TCFG_AUDIO_ANC_ENABLE && TCFG_AUDIO_ANC_ENV_ADAPTIVE_GAIN_ENABLE)
#include "system/includes.h"
#include "icsd_adt_app.h"
#include "audio_anc.h"
#include "asm/anc.h"
#include "tone_player.h"
#include "audio_config.h"
#include "icsd_anc_user.h"
#include "sniff.h"

#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE
#include "rt_anc_app.h"
#endif

#if 1
#define env_log	printf
#else
#define env_log(...)
#endif/*log_en*/

#define ICSD_ENV_LVL_PRINTF             1   //环境自适应阈值打印使能

struct audio_anc_env_adaptive_det {
    u8 lvl1_l_thr;
    // u8 lvl1_h_thr;
    u8 lvl2_l_thr;
    u8 lvl2_h_thr;
    // u8 lvl3_l_thr;
    u8 lvl3_h_thr;
    u8 cur_lvl;
    u8 last_lvl;
    u8 det_times;
    u8 cnt;
};

static struct audio_anc_env_adaptive_det anc_env_gain_det = {
    .lvl1_l_thr = 30,               //lvl l 低阈值
    .lvl2_l_thr = 70,               //lvl 2 低阈值
    .lvl2_h_thr = 50,               //lvl 2 高阈值
    .lvl3_h_thr = 90,               //lvl 3 高阈值
    .cur_lvl = ANC_WIND_NOISE_LVL2, //当前增益等级
    .last_lvl = ANC_WIND_NOISE_LVL2,//上次增益等级
    .det_times = 6,                 //连续性检测 时间 = avc_run_interval * 11ms * det_times
    .cnt = 0,
};

/*打开anc增益自适应*/
int audio_anc_env_adaptive_gain_open()
{
    env_log("%s", __func__);
    u8 adt_mode = ADT_ENV_NOISE_DET_MODE;
    return audio_icsd_adt_sync_open(adt_mode);
}

/*关闭anc增益自适应*/
int audio_anc_env_adaptive_gain_close()
{
    env_log("%s", __func__);
    u8 adt_mode = ADT_ENV_NOISE_DET_MODE;
    int ret = audio_icsd_adt_sync_close(adt_mode, 0);
    return ret;
}

/*anc增益自适应使用demo*/
void audio_anc_env_adaptive_gain_demo()
{
    env_log("%s", __func__);
    if (audio_icsd_adt_open_permit(ADT_ENV_NOISE_DET_MODE) == 0) {
        return;
    }

    if ((get_icsd_adt_mode() & ADT_ENV_NOISE_DET_MODE) == 0) {
        /*打开提示音*/
        /* icsd_adt_tone_play(ICSD_ADT_TONE_WINDDET_ON); */
        audio_anc_env_adaptive_gain_open();
    } else {
        /*关闭提示音*/
        /* icsd_adt_tone_play(ICSD_ADT_TONE_WINDDET_OFF); */
        audio_anc_env_adaptive_gain_close();
    }
}

static struct anc_fade_handle anc_env_gain_fade = {
    .timer_ms = 100, //配置定时器周期
    .timer_id = 0, //记录定时器id
    .cur_gain = 16384, //记录当前增益
    .fade_setp = 0,//记录淡入淡出步进
    .target_gain = 16384, //记录目标增益
    .fade_gain_mode = 0,//记录当前设置fade gain的模式
};

/*重置环境自适应增益的初始参数*/
void audio_anc_env_adaptive_fade_param_reset(void)
{
    anc_env_gain_det.cur_lvl = ANC_WIND_NOISE_LVL2;
    anc_env_gain_det.last_lvl = ANC_WIND_NOISE_LVL2;
    anc_env_gain_det.cnt = 0;

    if (anc_env_gain_fade.timer_id) {
        sys_timer_del(anc_env_gain_fade.timer_id);
        // sys_s_hi_timer_del(anc_env_gain_fade.timer_id);
        anc_env_gain_fade.timer_id = 0;
    }
    anc_env_gain_fade.cur_gain = 16384;
    anc_env_gain_fade.target_gain = 16384;
}

/*设置环境自适应增益，带淡入淡出功能*/
void audio_anc_env_adaptive_fade_gain_set(int fade_gain, int fade_time)
{
    audio_anc_gain_fade_process(&anc_env_gain_fade, ANC_FADE_MODE_ENV_ADAPTIVE_GAIN, fade_gain, fade_time);
}


int audio_env_noise_lvl_get(u8 cur_lvl, u8 spldb_iir)
{
    switch (cur_lvl) {
    case ANC_WIND_NOISE_LVL2:
        if (spldb_iir < anc_env_gain_det.lvl2_l_thr) {
            if (anc_env_gain_det.last_lvl == ANC_WIND_NOISE_LVL1) {
                if (++anc_env_gain_det.cnt >= anc_env_gain_det.det_times) {
                    cur_lvl = ANC_WIND_NOISE_LVL1;
                    anc_env_gain_det.cnt = 0;
                }
            } else {
                anc_env_gain_det.cnt = 0;
            }
            anc_env_gain_det.last_lvl = ANC_WIND_NOISE_LVL1;
        } else {
            anc_env_gain_det.cnt = 0;
        }
        break;
    case ANC_WIND_NOISE_LVL1:
        if (spldb_iir > anc_env_gain_det.lvl3_h_thr) {
            if (anc_env_gain_det.last_lvl == ANC_WIND_NOISE_LVL2) {
                if (++anc_env_gain_det.cnt >= anc_env_gain_det.det_times) {
                    cur_lvl = ANC_WIND_NOISE_LVL2;
                    anc_env_gain_det.cnt = 0;
                }
            } else {
                anc_env_gain_det.cnt = 0;
            }
            anc_env_gain_det.last_lvl = ANC_WIND_NOISE_LVL2;
        } else if (spldb_iir < anc_env_gain_det.lvl1_l_thr) {
            if (anc_env_gain_det.last_lvl == ANC_WIND_NOISE_LVL0) {
                if (++anc_env_gain_det.cnt >= anc_env_gain_det.det_times) {
                    cur_lvl = ANC_WIND_NOISE_LVL0;
                    anc_env_gain_det.cnt = 0;
                }
            } else {
                anc_env_gain_det.cnt = 0;
            }
            anc_env_gain_det.last_lvl = ANC_WIND_NOISE_LVL0;
        } else {
            anc_env_gain_det.cnt = 0;
        }
        break;
    case ANC_WIND_NOISE_LVL0:
        if (spldb_iir > anc_env_gain_det.lvl2_h_thr) {
            if (anc_env_gain_det.last_lvl == ANC_WIND_NOISE_LVL1) {
                if (++anc_env_gain_det.cnt >= anc_env_gain_det.det_times) {
                    cur_lvl = ANC_WIND_NOISE_LVL1;
                    anc_env_gain_det.cnt = 0;
                }
            } else {
                anc_env_gain_det.cnt = 0;
            }
            anc_env_gain_det.last_lvl = ANC_WIND_NOISE_LVL1;
        } else {
            anc_env_gain_det.cnt = 0;
        }
        break;
    }
    return cur_lvl;
}

/*环境噪声处理*/
int audio_env_noise_event_process(u8 spldb_iir)
{
    u8 anc_env_noise_lvl = 0;

    /*anc模式下才改anc增益*/
    if (anc_mode_get() != ANC_OFF) {
        /*划分环境噪声等级*/

        anc_env_gain_det.cur_lvl = audio_env_noise_lvl_get(anc_env_gain_det.cur_lvl, spldb_iir);
#if ICSD_ENV_LVL_PRINTF
        env_log("env_det-cur_lvl %d, spldb_iir local %d, tws_result %d:, cnt %d \n", anc_env_gain_det.cur_lvl, audio_anc_env_adaptive_cur_lvl(), spldb_iir, anc_env_gain_det.cnt);
#endif
        anc_env_noise_lvl = anc_env_gain_det.cur_lvl;

        if (anc_env_noise_lvl == 0) {
            return -1;
        }
    } else {
        return -1;
    }

    u16 anc_fade_gain = 16384;
    u16 anc_fade_time = 3000; //ms
    /*根据风噪等级改变anc增益*/
    switch (anc_env_noise_lvl) {
    case ANC_WIND_NOISE_LVL0:
        anc_fade_gain = (u16)(eq_db2mag(-20.0f) * ANC_FADE_GAIN_MAX_FLOAT);
        anc_fade_time = 3000;
        break;
    case ANC_WIND_NOISE_LVL1:
        anc_fade_gain = (u16)(eq_db2mag(-6.0f) * ANC_FADE_GAIN_MAX_FLOAT);;
        anc_fade_time = 3000;
        break;
    case ANC_WIND_NOISE_LVL2:
        anc_fade_gain = 16384;
        anc_fade_time = 3000;
        break;
    case ANC_WIND_NOISE_LVL3:
        anc_fade_gain = 16384;
        anc_fade_time = 3000;
        break;
    case ANC_WIND_NOISE_LVL4:
        anc_fade_gain = 16384;
        anc_fade_time = 3000;
        break;
    case ANC_WIND_NOISE_LVL5:
    case ANC_WIND_NOISE_LVL_MAX:
        anc_fade_gain = 16384;
        anc_fade_time = 3000;
        break;
    default:
        anc_fade_gain = 16384;
        anc_fade_time = 3000;
        break;
    }

    if (anc_env_gain_fade.target_gain == anc_fade_gain) {
        return anc_fade_gain;
    }
    env_log("ENV_NOISE_STATE:RUN, lvl %d, [anc_fade] %d, spldb %d\n", anc_env_noise_lvl, anc_fade_gain, spldb_iir);
    u8 data[5] = {0};
    data[0] = SYNC_ICSD_ADT_SET_ANC_FADE_GAIN;
    data[1] = anc_fade_gain & 0xff;
    data[2] = (anc_fade_gain >> 8) & 0xff;
    data[3] = ANC_FADE_MODE_ENV_ADAPTIVE_GAIN;
    data[4] = anc_fade_time / 100; //缩小100倍传参
    audio_icsd_adt_info_sync(data, 5);

    return anc_fade_gain;
}

#endif /*(defined TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN) && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
