#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".icsd_adt.data.bss")
#pragma data_seg(".icsd_adt.data")
#pragma const_seg(".icsd_adt.text.const")
#pragma code_seg(".icsd_adt.text")
#endif

#include "app_config.h"
#if ((defined TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN) && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN && \
        TCFG_AUDIO_ANC_ENABLE && TCFG_AUDIO_ANC_HOWLING_DET_ENABLE)
#include "icsd_adt.h"
#include "icsd_howl.h"

int (*howl_printf)(const char *format, ...) = _howl_printf;

void howl_config_init(__howl_config *_howl_config)
{
    _howl_config->hd_mic_sel = 0; // 0:REF 1:ERR
    _howl_config->hd_mode = 0; // 1 for anco, 0 for ref
    _howl_config->hd_scale = 0.2;//0.25 数字增益
    _howl_config->hd_sat_thr = 6800;//8000;//饱和阈值
    _howl_config->hd_det_thr = 4;//4;//满足条件 hd_det_thr次就触发 最小值为4
    _howl_config->hd_diff_pwr_thr = 11;//18;//越大反应越慢，误触会小影响hd_frame_max的成立
    _howl_config->hd_maxind_thr1 = 7;//频点1： hd_maxind_thr1 * 46875/128 小于这个频点要求更严，次数更多才触发
    _howl_config->hd_maxind_thr2 = 28;//频点2： hd_maxind_thr2 * 46875/128 大于这个频点要求更松，次数更少就触发
    _howl_config->hd_maxvld   = 120;
    _howl_config->hd_maxvld1  = 115;
    _howl_config->hd_maxvld2  = 130;
    _howl_config->hd_path1_thr = 110;
    _howl_config->hd_maxvld2nd  = 42;
    _howl_config->loud_thr    = 124; // 大声压认为是啸叫。
    _howl_config->loud_thr1   = 122; // 大声压认为是啸叫。
    _howl_config->loud_thr2   = 130; // 大声压认为是啸叫。
    _howl_config->hd_det_cnt  = 20; //cnt 超过该值 && maxvld，force是啸叫。
    _howl_config->hd_step_pwr = 4;  //最终功率增长 dB
    _howl_config->hd_rise_pwr = 88;
    _howl_config->hd_rise_cnt = 120;//长啸叫计数
}

const struct howl_function HOWL_FUNC_t = {
    .howl_config_init = howl_config_init,
    .anc_debug_malloc = anc_debug_malloc,
    .anc_debug_free = anc_debug_free,
};
struct howl_function *HOWL_FUNC = (struct howl_function *)(&HOWL_FUNC_t);

const u8 howl_log_en     = 1;
const u8 icsd_howl_debug = 1;
const u8 howl_debug_dlen = 10;//ramsize = (96 * howl_debug_dlen) byte

const u8 howl_ft_debug = 0;
#endif
