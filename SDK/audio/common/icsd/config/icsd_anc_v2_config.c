#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".icsd_anc.data.bss")
#pragma data_seg(".icsd_anc.data")
#pragma const_seg(".icsd_anc.text.const")
#pragma code_seg(".icsd_anc.text")
#endif

#define USE_BOARD_CONFIG 0
#define EXT_PRINTF_DEBUG 0

#include "app_config.h"
#include "audio_config_def.h"
#if ((TCFG_AUDIO_ANC_EAR_ADAPTIVE_EN || TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE) && \
	 TCFG_AUDIO_ANC_ENABLE && TCFG_AUDIO_ANC_EAR_ADAPTIVE_VERSION == ANC_EXT_V2)

#include "audio_anc.h"
#include "icsd_anc_v2_config.h"
#include "icsd_common_v2.h"

#if (USE_BOARD_CONFIG == 1)
#include "icsd_anc_v2_board.c"
#endif

#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE
#include "rt_anc_app.h"
#endif

const u8 ICSD_ANC_VERSION = 2;
const u8 ICSD_ANC_TOOL_PRINTF = 0;
const u8 msedif_en = 0;
const u8 target_diff_en = 0;

struct icsd_anc_v2_tool_data *TOOL_DATA = NULL;

const int cmp_idx_begin = 0;
const int cmp_idx_end = 59;
const int cmp_total_len = 60;
const int cmp_en = 0;
const float pz_gain = 0;
const u8 ICSD_ANC_V2_BYPASS_ON_FIRST = 0;//播放提示音过程中打开BYPASS节省BYPASS稳定时间

/***************************************************************/
/****************** ANC MODE bypass fgq ************************/
/***************************************************************/
const float gfq_bypass[] = {
    -10,  5000,  1,
};
const u8 tap_bypass = 1;
const u8 type_bypass[] = {3};
const float fs_bypass = 375e3;
double bbbaa_bypass[1 * 5];

void icsd_anc_v2_board_config()
{
    double a0, a1, a2, b0, b1, b2;
    for (int i = 0; i < tap_bypass; i++) {
        icsd_biquad2ab_out_v2(gfq_bypass[3 * i], gfq_bypass[3 * i + 1], fs_bypass, gfq_bypass[3 * i + 2], &a0, &a1, &a2, &b0, &b1, &b2, type_bypass[i]);
        bbbaa_bypass[5 * i + 0] = b0 / a0;
        bbbaa_bypass[5 * i + 1] = b1 / a0;
        bbbaa_bypass[5 * i + 2] = b2 / a0;
        bbbaa_bypass[5 * i + 3] = a1 / a0;
        bbbaa_bypass[5 * i + 4] = a2 / a0;
    }
}

/***************************************************************/
/**************************** FF *******************************/
/***************************************************************/
const int FF_objFunc_type  = 3;

const float FSTOP_IDX = 2700;
const float FSTOP_IDX2 = 2700;

const float Gold_csv_Perf_Range[] = {
    2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,
    2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,
    2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,
    2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,
};

//degree_level, gain_limit, limit_mse_begin, limit_mse_end, over_mse_begin, over_mse_end,biquad_cut
float degree_set0[] = {3, -3, 50, 2200, 50, 2700, 5};
float degree_set1[] = {8, 10, 50, 2200, 50, 2700, 8};
float degree_set2[] = {11, 5, 50, 2200, 50, 2700, 11};


#if EXT_PRINTF_DEBUG
static void de_vrange_printf(float *vrange, int order)
{
    printf(" ff oreder = %d\n", order);
    for (int i = 0; i < order; i++) {
        printf("%d >>>>>>>>>>> g:%d, %d, f:%d, %d, q:%d, %d\n", i, (int)(vrange[6 * i + 0] * 1000), (int)(vrange[6 * i + 1] * 1000)
               , (int)(vrange[6 * i + 2] * 1000), (int)(vrange[6 * i + 3] * 1000)
               , (int)(vrange[6 * i + 4] * 1000), (int)(vrange[6 * i + 5] * 1000));
    }
}

static void de_biquad_printf(float *biquad, int order)
{
    printf(" ff oreder = %d\n", order);
    for (int i = 0; i < order; i++) {
        printf("%d >>>>> g:%d, f:%d, q:%d\n", i, (int)(biquad[3 * i + 0] * 1000), (int)(biquad[3 * i + 1]), (int)(biquad[3 * i + 2] * 1000));
    }
    printf("total gain = %d\n", (int)(biquad[order * 3] * 1000));
}
#endif

const u8 mem_list[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 21, 23, 25, 27, 31, 35, 39, 43, 51, 59};

// ref_gain: liner
// err_gain: liner
// eq_freq range: 40 --> 2.7khz
static void szpz_cmp_get(float *pz_cmp, float *sz_cmp, float ref_gain, float err_gain, struct aeq_default_seg_tab *eq_par, float eq_freq_l, float eq_freq_h, u8 eq_total_gain_en)
{
    const int len = 25;
    float *freq = malloc(len * 4);
    float *eq_hz = malloc(len * 8);
    float *iir_ab = malloc(5 * 10 * 4);	 // eq tap <= 10

    ASSERT(pz_cmp, "pz_cmp input err\n");
    ASSERT(sz_cmp, "sz_cmp input err\n");

    for (int i = 0; i < len; i++) {
        freq[i] = 46875.0 / 1024 * mem_list[i];
        pz_cmp[2 * i + 0] = 1;
        pz_cmp[2 * i + 1] = 0;
        sz_cmp[2 * i + 0] = 1;
        sz_cmp[2 * i + 1] = 0;
    }

    u8 eq_idx_l = 0;
    u8 eq_idx_h = 0;
    for (int i = 0; i < len - 1; i++) {
        if ((eq_freq_l > freq[i]) && (eq_freq_l <= freq[i + 1])) {
            eq_idx_l = i;
        }
        if ((eq_freq_h > freq[i]) && (eq_freq_h <= freq[i + 1])) {
            eq_idx_h = i;
        }
    }
    //printf("freq:%d, %d; ind:%d, %d\n", (int)eq_freq_l, (int)eq_freq_h, eq_idx_l, eq_idx_h);
    if (eq_par) {
        icsd_aeq_fgq2eq(eq_par, iir_ab, eq_hz, freq, 46875, len * 2);
    } else {
        for (int i = 0; i < len; i++) {
            eq_hz[2 * i + 0] = 1;
            eq_hz[2 * i + 1] = 0;
        }
    }

    //for(int i=0; i<len; i++){
    //    printf("%d, %d\n", (int)(eq_hz[2*i]*1e6), (int)(eq_hz[2*i+1]));
    //}


    if (eq_total_gain_en) {
        float eq_gain = 1;
        float cnt = 0;
        float db = 0;
        for (int i = eq_idx_l; i < eq_idx_h; i++) {
            db += 10 * icsd_log10_anc(eq_hz[2 * i] * eq_hz[2 * i] + eq_hz[2 * i + 1] * eq_hz[2 * i + 1]);
            cnt = cnt + 1;
        }
        if (cnt != 0) {
            db = db / cnt;
        }
        eq_gain = eq_gain * icsd_anc_pow10(db / 20);

        float pz_cmp_gain = err_gain;
        float sz_cmp_gain = err_gain;

        if (ref_gain != 0) {
            pz_cmp_gain = err_gain / ref_gain;
        }

        if (eq_gain != 0) {
            sz_cmp_gain = err_gain / eq_gain;
        }

        for (int i = 0; i < len; i++) {
            pz_cmp[2 * i + 0] = pz_cmp_gain;
            pz_cmp[2 * i + 1] = 0;
            sz_cmp[2 * i + 0] = sz_cmp_gain;
            sz_cmp[2 * i + 1] = 0;
        }
    } else {
        float err_tmp[2];
        err_tmp[0] = err_gain;
        err_tmp[1] = 0;

        for (int i = 0; i < len; i++) {
            pz_cmp[2 * i + 0] = err_gain / ref_gain;
            pz_cmp[2 * i + 1] = 0;
            icsd_complex_div_v2(err_tmp, &eq_hz[2 * i], &sz_cmp[2 * i], 2);
        }
    }

    free(freq);
    free(eq_hz);
    free(iir_ab);

}

void icsd_sd_cfg_set(__icsd_anc_config_data *SD_CFG, void *_ext_cfg)
{
    struct anc_ext_ear_adaptive_param *ext_cfg = (struct anc_ext_ear_adaptive_param *)_ext_cfg;
#if (USE_BOARD_CONFIG == 1)

    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>anc use board config\n");
    icsd_anc_config_board_init(SD_CFG);

    return ;
#endif
    if (SD_CFG) {
        //界面信息配置
        SD_CFG->pnc_times = ext_cfg->base_cfg->adaptive_times;
        SD_CFG->vld1 = ext_cfg->base_cfg->vld1;
        SD_CFG->vld2 = ext_cfg->base_cfg->vld2;
        SD_CFG->sz_pri_thr = ext_cfg->base_cfg->sz_pri_thr;
        SD_CFG->bypass_vol = ext_cfg->base_cfg->bypass_vol;
        SD_CFG->sz_calr_sign = ext_cfg->base_cfg->calr_sign[0];
        SD_CFG->pz_calr_sign = ext_cfg->base_cfg->calr_sign[1];
        SD_CFG->bypass_calr_sign = ext_cfg->base_cfg->calr_sign[2];
        SD_CFG->perf_calr_sign = ext_cfg->base_cfg->calr_sign[3];
        SD_CFG->train_mode = ext_cfg->train_mode;	//自适应训练模式设置 */
        SD_CFG->tonel_delay = ext_cfg->base_cfg->tonel_delay;
        SD_CFG->toner_delay = ext_cfg->base_cfg->toner_delay;
        SD_CFG->pzl_delay = ext_cfg->base_cfg->pzl_delay;
        SD_CFG->pzr_delay = ext_cfg->base_cfg->pzr_delay;
        SD_CFG->ear_recorder = ext_cfg->ff_ear_mem_param->ear_recorder;
        SD_CFG->fb_agc_en = 0;
        //其他配置
        SD_CFG->ff_yorder  = ANC_ADAPTIVE_FF_ORDER;
        SD_CFG->fb_yorder  = ANC_ADAPTIVE_FB_ORDER;
        //SD_CFG->cmp_yorder = ANC_ADAPTIVE_CMP_ORDER;
        if (ICSD_ANC_CPU == ICSD_BR28) {
            printf("ICSD BR28 SEL\n");
            SD_CFG->normal_out_sel_l = BR28_ANC_USER_TRAIN_OUT_SEL_L;
            SD_CFG->normal_out_sel_r = BR28_ANC_USER_TRAIN_OUT_SEL_R;
            SD_CFG->tone_out_sel_l   = BR28_ANC_TONE_TRAIN_OUT_SEL_L;
            SD_CFG->tone_out_sel_r   = BR28_ANC_TONE_TRAIN_OUT_SEL_R;
        } else if (ICSD_ANC_CPU == ICSD_BR50) {
            printf("ICSD BR50 SEL\n");
            SD_CFG->normal_out_sel_l = BR50_ANC_USER_TRAIN_OUT_SEL_L;
            SD_CFG->normal_out_sel_r = BR50_ANC_USER_TRAIN_OUT_SEL_R;
            SD_CFG->tone_out_sel_l   = BR50_ANC_TONE_TRAIN_OUT_SEL_L;
            SD_CFG->tone_out_sel_r   = BR50_ANC_TONE_TRAIN_OUT_SEL_R;
        } else {
            printf("CPU ERR\n");
        }
        /***************************************************/
        /**************** left channel config **************/
        /***************************************************/
        SD_CFG->adpt_cfg.pnc_times = ext_cfg->base_cfg->adaptive_times;

        // de alg config
        SD_CFG->adpt_cfg.high_fgq_fix = 1; // gali TODO  需要工具做传参
        SD_CFG->adpt_cfg.de_alg_sel = 1; // gali TODO

        // target配置
        SD_CFG->adpt_cfg.cmp_en = ext_cfg->ff_target_param->cmp_en;
        SD_CFG->adpt_cfg.target_cmp_num = ext_cfg->ff_target_param->cmp_curve_num;
        SD_CFG->adpt_cfg.target_sv = ext_cfg->ff_target_sv->data;
        SD_CFG->adpt_cfg.target_cmp_dat = ext_cfg->ff_target_cmp->data;

        // 算法配置
        SD_CFG->adpt_cfg.IIR_NUM_FLEX = 0;
        int flex_idx = 0;
        int biquad_idx = 0;
        for (int i = 0; i < SD_CFG->ff_yorder; i++) {
            if (ext_cfg->ff_iir_high[i].fixed_en) { // fix
                SD_CFG->adpt_cfg.Biquad_init_H[3 * biquad_idx + 0] = ext_cfg->ff_iir_high[i].def.gain;
                SD_CFG->adpt_cfg.Biquad_init_H[3 * biquad_idx + 1] = ext_cfg->ff_iir_high[i].def.fre;
                SD_CFG->adpt_cfg.Biquad_init_H[3 * biquad_idx + 2] = ext_cfg->ff_iir_high[i].def.q;
                SD_CFG->adpt_cfg.Biquad_init_M[3 * biquad_idx + 0] = ext_cfg->ff_iir_medium[i].def.gain;
                SD_CFG->adpt_cfg.Biquad_init_M[3 * biquad_idx + 1] = ext_cfg->ff_iir_medium[i].def.fre;
                SD_CFG->adpt_cfg.Biquad_init_M[3 * biquad_idx + 2] = ext_cfg->ff_iir_medium[i].def.q;
                SD_CFG->adpt_cfg.Biquad_init_L[3 * biquad_idx + 0] = ext_cfg->ff_iir_low[i].def.gain;
                SD_CFG->adpt_cfg.Biquad_init_L[3 * biquad_idx + 1] = ext_cfg->ff_iir_low[i].def.fre;
                SD_CFG->adpt_cfg.Biquad_init_L[3 * biquad_idx + 2] = ext_cfg->ff_iir_low[i].def.q;
                SD_CFG->adpt_cfg.biquad_type[biquad_idx] = ext_cfg->ff_iir_high[i].type;
                biquad_idx += 1;
            } else { // not fix
                SD_CFG->adpt_cfg.Vrange_H[6 * flex_idx + 0] = ext_cfg->ff_iir_high[i].lower_limit.gain;
                SD_CFG->adpt_cfg.Vrange_H[6 * flex_idx + 1] = ext_cfg->ff_iir_high[i].upper_limit.gain;
                SD_CFG->adpt_cfg.Vrange_H[6 * flex_idx + 2] = ext_cfg->ff_iir_high[i].lower_limit.fre;
                SD_CFG->adpt_cfg.Vrange_H[6 * flex_idx + 3] = ext_cfg->ff_iir_high[i].upper_limit.fre;
                SD_CFG->adpt_cfg.Vrange_H[6 * flex_idx + 4] = ext_cfg->ff_iir_high[i].lower_limit.q;
                SD_CFG->adpt_cfg.Vrange_H[6 * flex_idx + 5] = ext_cfg->ff_iir_high[i].upper_limit.q;
                SD_CFG->adpt_cfg.Vrange_M[6 * flex_idx + 0] = ext_cfg->ff_iir_medium[i].lower_limit.gain;
                SD_CFG->adpt_cfg.Vrange_M[6 * flex_idx + 1] = ext_cfg->ff_iir_medium[i].upper_limit.gain;
                SD_CFG->adpt_cfg.Vrange_M[6 * flex_idx + 2] = ext_cfg->ff_iir_medium[i].lower_limit.fre;
                SD_CFG->adpt_cfg.Vrange_M[6 * flex_idx + 3] = ext_cfg->ff_iir_medium[i].upper_limit.fre;
                SD_CFG->adpt_cfg.Vrange_M[6 * flex_idx + 4] = ext_cfg->ff_iir_medium[i].lower_limit.q;
                SD_CFG->adpt_cfg.Vrange_M[6 * flex_idx + 5] = ext_cfg->ff_iir_medium[i].upper_limit.q;
                SD_CFG->adpt_cfg.Vrange_L[6 * flex_idx + 0] = ext_cfg->ff_iir_low[i].lower_limit.gain;
                SD_CFG->adpt_cfg.Vrange_L[6 * flex_idx + 1] = ext_cfg->ff_iir_low[i].upper_limit.gain;
                SD_CFG->adpt_cfg.Vrange_L[6 * flex_idx + 2] = ext_cfg->ff_iir_low[i].lower_limit.fre;
                SD_CFG->adpt_cfg.Vrange_L[6 * flex_idx + 3] = ext_cfg->ff_iir_low[i].upper_limit.fre;
                SD_CFG->adpt_cfg.Vrange_L[6 * flex_idx + 4] = ext_cfg->ff_iir_low[i].lower_limit.q;
                SD_CFG->adpt_cfg.Vrange_L[6 * flex_idx + 5] = ext_cfg->ff_iir_low[i].upper_limit.q;
                flex_idx += 1;
            }
        }

        SD_CFG->adpt_cfg.Vrange_H[flex_idx * 6 + 0] = -ext_cfg->ff_iir_high_gains->lower_limit_gain;
        SD_CFG->adpt_cfg.Vrange_H[flex_idx * 6 + 1] = -ext_cfg->ff_iir_high_gains->upper_limit_gain;
        SD_CFG->adpt_cfg.Vrange_M[flex_idx * 6 + 0] = -ext_cfg->ff_iir_medium_gains->lower_limit_gain;
        SD_CFG->adpt_cfg.Vrange_M[flex_idx * 6 + 1] = -ext_cfg->ff_iir_medium_gains->upper_limit_gain;
        SD_CFG->adpt_cfg.Vrange_L[flex_idx * 6 + 0] = -ext_cfg->ff_iir_low_gains->lower_limit_gain;
        SD_CFG->adpt_cfg.Vrange_L[flex_idx * 6 + 1] = -ext_cfg->ff_iir_low_gains->upper_limit_gain;


        SD_CFG->adpt_cfg.IIR_NUM_FLEX = flex_idx;
        SD_CFG->adpt_cfg.IIR_NUM_FIX = biquad_idx;

        for (int i = 0; i < SD_CFG->ff_yorder; i++) {
            if (!ext_cfg->ff_iir_high[i].fixed_en) { // not fix
                SD_CFG->adpt_cfg.Biquad_init_H[3 * biquad_idx + 0] = ext_cfg->ff_iir_high[i].def.gain;
                SD_CFG->adpt_cfg.Biquad_init_H[3 * biquad_idx + 1] = ext_cfg->ff_iir_high[i].def.fre;
                SD_CFG->adpt_cfg.Biquad_init_H[3 * biquad_idx + 2] = ext_cfg->ff_iir_high[i].def.q;
                SD_CFG->adpt_cfg.Biquad_init_M[3 * biquad_idx + 0] = ext_cfg->ff_iir_medium[i].def.gain;
                SD_CFG->adpt_cfg.Biquad_init_M[3 * biquad_idx + 1] = ext_cfg->ff_iir_medium[i].def.fre;
                SD_CFG->adpt_cfg.Biquad_init_M[3 * biquad_idx + 2] = ext_cfg->ff_iir_medium[i].def.q;
                SD_CFG->adpt_cfg.Biquad_init_L[3 * biquad_idx + 0] = ext_cfg->ff_iir_low[i].def.gain;
                SD_CFG->adpt_cfg.Biquad_init_L[3 * biquad_idx + 1] = ext_cfg->ff_iir_low[i].def.fre;
                SD_CFG->adpt_cfg.Biquad_init_L[3 * biquad_idx + 2] = ext_cfg->ff_iir_low[i].def.q;
                SD_CFG->adpt_cfg.biquad_type[biquad_idx] = ext_cfg->ff_iir_high[i].type;
                biquad_idx += 1;
            }
        }

        SD_CFG->adpt_cfg.Biquad_init_H[biquad_idx * 3] = -ext_cfg->ff_iir_high_gains->def_total_gain;
        SD_CFG->adpt_cfg.Biquad_init_M[biquad_idx * 3] = -ext_cfg->ff_iir_medium_gains->def_total_gain;
        SD_CFG->adpt_cfg.Biquad_init_L[biquad_idx * 3] = -ext_cfg->ff_iir_low_gains->def_total_gain;

        for (int i = 0; i < 7; i++) {
            SD_CFG->adpt_cfg.degree_set0[i] = degree_set0[i];
            SD_CFG->adpt_cfg.degree_set1[i] = degree_set1[i];
            SD_CFG->adpt_cfg.degree_set2[i] = degree_set2[i];
        }

        SD_CFG->adpt_cfg.degree_set0[0] = ext_cfg->ff_iir_general->biquad_level_l;
        SD_CFG->adpt_cfg.degree_set1[0] = ext_cfg->ff_iir_general->biquad_level_h;
        SD_CFG->adpt_cfg.degree_set2[0] = ext_cfg->ff_target_param->cmp_curve_num;
        SD_CFG->adpt_cfg.degree_set0[6] = ext_cfg->ff_iir_general->biquad_level_l;
        SD_CFG->adpt_cfg.degree_set1[6] = ext_cfg->ff_iir_general->biquad_level_h;
        SD_CFG->adpt_cfg.degree_set2[6] = ext_cfg->ff_target_param->cmp_curve_num;

        SD_CFG->adpt_cfg.degree_set0[1] = ext_cfg->ff_iir_high_gains->iir_target_gain_limit;
        SD_CFG->adpt_cfg.degree_set1[1] = ext_cfg->ff_iir_medium_gains->iir_target_gain_limit;
        SD_CFG->adpt_cfg.degree_set2[1] = ext_cfg->ff_iir_low_gains->iir_target_gain_limit;

        SD_CFG->adpt_cfg.Weight_H = ext_cfg->ff_weight_high->data;
        SD_CFG->adpt_cfg.Weight_M = ext_cfg->ff_weight_medium->data;
        SD_CFG->adpt_cfg.Weight_L = ext_cfg->ff_weight_low->data;
        SD_CFG->adpt_cfg.Gold_csv_H = ext_cfg->ff_mse_high->data;
        SD_CFG->adpt_cfg.Gold_csv_M = ext_cfg->ff_mse_medium->data;
        SD_CFG->adpt_cfg.Gold_csv_L = ext_cfg->ff_mse_low->data;

        SD_CFG->adpt_cfg.total_gain_adj_begin = ext_cfg->ff_iir_general->total_gain_freq_l;
        SD_CFG->adpt_cfg.total_gain_adj_end = ext_cfg->ff_iir_general->total_gain_freq_h;
        SD_CFG->adpt_cfg.gain_limit_all = ext_cfg->ff_iir_general->total_gain_limit;

        // 耳道记忆曲线配置
#if AUDIO_RT_ANC_SZ_PZ_CMP_EN
        SD_CFG->adpt_cfg.pz_table_cmp = audio_rtanc_pz_cmp_get();
        SD_CFG->adpt_cfg.sz_table_cmp = audio_rtanc_sz_cmp_get();
#else
        SD_CFG->adpt_cfg.pz_table_cmp = NULL;
        SD_CFG->adpt_cfg.sz_table_cmp = NULL;
#endif
        SD_CFG->adpt_cfg.mem_curve_nums = ext_cfg->ff_ear_mem_param->mem_curve_nums;
        SD_CFG->adpt_cfg.sz_table = ext_cfg->ff_ear_mem_sz->data;
        SD_CFG->adpt_cfg.pz_table = ext_cfg->ff_ear_mem_pz->data;

        //临时测试流程
#if AUDIO_RT_ANC_SELF_TALK_FLAG  // test self talk coeff
        extern const float pz_coef_table[];
        extern const float sz_coef_table[];
        SD_CFG->adpt_cfg.pz_coef_table = (float *)pz_coef_table;
        SD_CFG->adpt_cfg.sz_coef_table = (float *)sz_coef_table;
#else
        SD_CFG->adpt_cfg.pz_coef_table = NULL;
        SD_CFG->adpt_cfg.sz_coef_table = NULL;
#endif

#if EXT_PRINTF_DEBUG
        for (int i = 0; i < 60; i++) {
            printf("idx=%d, mse=%d, weight=%d\n", i, (int)SD_CFG->adpt_cfg.Gold_csv_H[i], (int)SD_CFG->adpt_cfg.Weight_H[i]);
        }
        for (int i = 0; i < 60; i++) {
            printf("idx=%d, mse=%d, weight=%d\n", i, (int)SD_CFG->adpt_cfg.Gold_csv_M[i], (int)SD_CFG->adpt_cfg.Weight_M[i]);
        }
        for (int i = 0; i < 60; i++) {
            printf("idx=%d, mse=%d, weight=%d\n", i, (int)SD_CFG->adpt_cfg.Gold_csv_L[i], (int)SD_CFG->adpt_cfg.Weight_L[i]);
        }

        de_vrange_printf(SD_CFG->adpt_cfg.Vrange_H, SD_CFG->adpt_cfg.IIR_NUM_FLEX + SD_CFG->adpt_cfg.IIR_NUM_FIX);
        de_vrange_printf(SD_CFG->adpt_cfg.Vrange_M, SD_CFG->adpt_cfg.IIR_NUM_FLEX + SD_CFG->adpt_cfg.IIR_NUM_FIX);
        de_vrange_printf(SD_CFG->adpt_cfg.Vrange_L, SD_CFG->adpt_cfg.IIR_NUM_FLEX + SD_CFG->adpt_cfg.IIR_NUM_FIX);

        de_biquad_printf(SD_CFG->adpt_cfg.Biquad_init_H, SD_CFG->adpt_cfg.IIR_NUM_FLEX + SD_CFG->adpt_cfg.IIR_NUM_FIX);
        de_biquad_printf(SD_CFG->adpt_cfg.Biquad_init_M, SD_CFG->adpt_cfg.IIR_NUM_FLEX + SD_CFG->adpt_cfg.IIR_NUM_FIX);
        de_biquad_printf(SD_CFG->adpt_cfg.Biquad_init_L, SD_CFG->adpt_cfg.IIR_NUM_FLEX + SD_CFG->adpt_cfg.IIR_NUM_FIX);
#endif

#if TCFG_AUDIO_ANC_CH == (ANC_L_CH | ANC_R_CH)
        /***************************************************/
        /*************** right channel config **************/
        /***************************************************/
        SD_CFG->adpt_cfg_r.pnc_times = ext_cfg->base_cfg->adaptive_times;

        // de alg config
        SD_CFG->adpt_cfg_r.high_fgq_fix = 1; // gali TODO
        SD_CFG->adpt_cfg_r.de_alg_sel = 1; // TODO

        // target配置
        SD_CFG->adpt_cfg_r.cmp_en = ext_cfg->rff_target_param->cmp_en;
        SD_CFG->adpt_cfg_r.target_cmp_num = ext_cfg->rff_target_param->cmp_curve_num;
        SD_CFG->adpt_cfg_r.target_sv = ext_cfg->rff_target_sv->data;
        SD_CFG->adpt_cfg_r.target_cmp_dat = ext_cfg->rff_target_cmp->data;

        // 算法配置
        SD_CFG->adpt_cfg_r.IIR_NUM_FLEX = 0;
        flex_idx = 0;
        biquad_idx = 0;
        for (int i = 0; i < SD_CFG->ff_yorder; i++) {
            if (ext_cfg->rff_iir_high[i].fixed_en) { // fix
                SD_CFG->adpt_cfg_r.Biquad_init_H[3 * biquad_idx + 0] = ext_cfg->rff_iir_high[i].def.gain;
                SD_CFG->adpt_cfg_r.Biquad_init_H[3 * biquad_idx + 1] = ext_cfg->rff_iir_high[i].def.fre;
                SD_CFG->adpt_cfg_r.Biquad_init_H[3 * biquad_idx + 2] = ext_cfg->rff_iir_high[i].def.q;
                SD_CFG->adpt_cfg_r.Biquad_init_M[3 * biquad_idx + 0] = ext_cfg->rff_iir_medium[i].def.gain;
                SD_CFG->adpt_cfg_r.Biquad_init_M[3 * biquad_idx + 1] = ext_cfg->rff_iir_medium[i].def.fre;
                SD_CFG->adpt_cfg_r.Biquad_init_M[3 * biquad_idx + 2] = ext_cfg->rff_iir_medium[i].def.q;
                SD_CFG->adpt_cfg_r.Biquad_init_L[3 * biquad_idx + 0] = ext_cfg->rff_iir_low[i].def.gain;
                SD_CFG->adpt_cfg_r.Biquad_init_L[3 * biquad_idx + 1] = ext_cfg->rff_iir_low[i].def.fre;
                SD_CFG->adpt_cfg_r.Biquad_init_L[3 * biquad_idx + 2] = ext_cfg->rff_iir_low[i].def.q;
                SD_CFG->adpt_cfg_r.biquad_type[biquad_idx] = ext_cfg->rff_iir_high[i].type;
                biquad_idx += 1;
            } else { // not fix
                SD_CFG->adpt_cfg_r.Vrange_H[6 * flex_idx + 0] = ext_cfg->rff_iir_high[i].lower_limit.gain;
                SD_CFG->adpt_cfg_r.Vrange_H[6 * flex_idx + 1] = ext_cfg->rff_iir_high[i].upper_limit.gain;
                SD_CFG->adpt_cfg_r.Vrange_H[6 * flex_idx + 2] = ext_cfg->rff_iir_high[i].lower_limit.fre;
                SD_CFG->adpt_cfg_r.Vrange_H[6 * flex_idx + 3] = ext_cfg->rff_iir_high[i].upper_limit.fre;
                SD_CFG->adpt_cfg_r.Vrange_H[6 * flex_idx + 4] = ext_cfg->rff_iir_high[i].lower_limit.q;
                SD_CFG->adpt_cfg_r.Vrange_H[6 * flex_idx + 5] = ext_cfg->rff_iir_high[i].upper_limit.q;
                SD_CFG->adpt_cfg_r.Vrange_M[6 * flex_idx + 0] = ext_cfg->rff_iir_medium[i].lower_limit.gain;
                SD_CFG->adpt_cfg_r.Vrange_M[6 * flex_idx + 1] = ext_cfg->rff_iir_medium[i].upper_limit.gain;
                SD_CFG->adpt_cfg_r.Vrange_M[6 * flex_idx + 2] = ext_cfg->rff_iir_medium[i].lower_limit.fre;
                SD_CFG->adpt_cfg_r.Vrange_M[6 * flex_idx + 3] = ext_cfg->rff_iir_medium[i].upper_limit.fre;
                SD_CFG->adpt_cfg_r.Vrange_M[6 * flex_idx + 4] = ext_cfg->rff_iir_medium[i].lower_limit.q;
                SD_CFG->adpt_cfg_r.Vrange_M[6 * flex_idx + 5] = ext_cfg->rff_iir_medium[i].upper_limit.q;
                SD_CFG->adpt_cfg_r.Vrange_L[6 * flex_idx + 0] = ext_cfg->rff_iir_low[i].lower_limit.gain;
                SD_CFG->adpt_cfg_r.Vrange_L[6 * flex_idx + 1] = ext_cfg->rff_iir_low[i].upper_limit.gain;
                SD_CFG->adpt_cfg_r.Vrange_L[6 * flex_idx + 2] = ext_cfg->rff_iir_low[i].lower_limit.fre;
                SD_CFG->adpt_cfg_r.Vrange_L[6 * flex_idx + 3] = ext_cfg->rff_iir_low[i].upper_limit.fre;
                SD_CFG->adpt_cfg_r.Vrange_L[6 * flex_idx + 4] = ext_cfg->rff_iir_low[i].lower_limit.q;
                SD_CFG->adpt_cfg_r.Vrange_L[6 * flex_idx + 5] = ext_cfg->rff_iir_low[i].upper_limit.q;
                flex_idx += 1;
            }
        }

        SD_CFG->adpt_cfg_r.Vrange_H[flex_idx * 6 + 0] = -ext_cfg->rff_iir_high_gains->lower_limit_gain;
        SD_CFG->adpt_cfg_r.Vrange_H[flex_idx * 6 + 1] = -ext_cfg->rff_iir_high_gains->upper_limit_gain;
        SD_CFG->adpt_cfg_r.Vrange_M[flex_idx * 6 + 0] = -ext_cfg->rff_iir_medium_gains->lower_limit_gain;
        SD_CFG->adpt_cfg_r.Vrange_M[flex_idx * 6 + 1] = -ext_cfg->rff_iir_medium_gains->upper_limit_gain;
        SD_CFG->adpt_cfg_r.Vrange_L[flex_idx * 6 + 0] = -ext_cfg->rff_iir_low_gains->lower_limit_gain;
        SD_CFG->adpt_cfg_r.Vrange_L[flex_idx * 6 + 1] = -ext_cfg->rff_iir_low_gains->upper_limit_gain;

        SD_CFG->adpt_cfg_r.IIR_NUM_FLEX = flex_idx;
        SD_CFG->adpt_cfg_r.IIR_NUM_FIX = biquad_idx;

        for (int i = 0; i < SD_CFG->ff_yorder; i++) {
            if (!ext_cfg->rff_iir_high[i].fixed_en) { // not fix
                SD_CFG->adpt_cfg_r.Biquad_init_H[3 * biquad_idx + 0] = ext_cfg->rff_iir_high[i].def.gain;
                SD_CFG->adpt_cfg_r.Biquad_init_H[3 * biquad_idx + 1] = ext_cfg->rff_iir_high[i].def.fre;
                SD_CFG->adpt_cfg_r.Biquad_init_H[3 * biquad_idx + 2] = ext_cfg->rff_iir_high[i].def.q;
                SD_CFG->adpt_cfg_r.Biquad_init_M[3 * biquad_idx + 0] = ext_cfg->rff_iir_medium[i].def.gain;
                SD_CFG->adpt_cfg_r.Biquad_init_M[3 * biquad_idx + 1] = ext_cfg->rff_iir_medium[i].def.fre;
                SD_CFG->adpt_cfg_r.Biquad_init_M[3 * biquad_idx + 2] = ext_cfg->rff_iir_medium[i].def.q;
                SD_CFG->adpt_cfg_r.Biquad_init_L[3 * biquad_idx + 0] = ext_cfg->rff_iir_low[i].def.gain;
                SD_CFG->adpt_cfg_r.Biquad_init_L[3 * biquad_idx + 1] = ext_cfg->rff_iir_low[i].def.fre;
                SD_CFG->adpt_cfg_r.Biquad_init_L[3 * biquad_idx + 2] = ext_cfg->rff_iir_low[i].def.q;
                SD_CFG->adpt_cfg_r.biquad_type[biquad_idx] = ext_cfg->rff_iir_high[i].type;
                biquad_idx += 1;
            }
        }

        SD_CFG->adpt_cfg_r.Biquad_init_H[biquad_idx * 3] = -ext_cfg->rff_iir_high_gains->def_total_gain;
        SD_CFG->adpt_cfg_r.Biquad_init_M[biquad_idx * 3] = -ext_cfg->rff_iir_medium_gains->def_total_gain;
        SD_CFG->adpt_cfg_r.Biquad_init_L[biquad_idx * 3] = -ext_cfg->rff_iir_low_gains->def_total_gain;

        for (int i = 0; i < 7; i++) {
            SD_CFG->adpt_cfg_r.degree_set0[i] = degree_set0[i];
            SD_CFG->adpt_cfg_r.degree_set1[i] = degree_set1[i];
            SD_CFG->adpt_cfg_r.degree_set2[i] = degree_set2[i];
        }

        SD_CFG->adpt_cfg_r.degree_set0[0] = ext_cfg->rff_iir_general->biquad_level_l;
        SD_CFG->adpt_cfg_r.degree_set1[0] = ext_cfg->rff_iir_general->biquad_level_h;
        SD_CFG->adpt_cfg_r.degree_set2[0] = ext_cfg->rff_target_param->cmp_curve_num;
        SD_CFG->adpt_cfg_r.degree_set0[6] = ext_cfg->rff_iir_general->biquad_level_l;
        SD_CFG->adpt_cfg_r.degree_set1[6] = ext_cfg->rff_iir_general->biquad_level_h;
        SD_CFG->adpt_cfg_r.degree_set2[6] = ext_cfg->rff_target_param->cmp_curve_num;

        SD_CFG->adpt_cfg_r.degree_set0[1] = ext_cfg->rff_iir_high_gains->iir_target_gain_limit;
        SD_CFG->adpt_cfg_r.degree_set1[1] = ext_cfg->rff_iir_medium_gains->iir_target_gain_limit;
        SD_CFG->adpt_cfg_r.degree_set2[1] = ext_cfg->rff_iir_low_gains->iir_target_gain_limit;

        SD_CFG->adpt_cfg_r.Weight_H = ext_cfg->rff_weight_high->data;
        SD_CFG->adpt_cfg_r.Weight_M = ext_cfg->rff_weight_medium->data;
        SD_CFG->adpt_cfg_r.Weight_L = ext_cfg->rff_weight_low->data;
        SD_CFG->adpt_cfg_r.Gold_csv_H = ext_cfg->rff_mse_high->data;
        SD_CFG->adpt_cfg_r.Gold_csv_M = ext_cfg->rff_mse_medium->data;
        SD_CFG->adpt_cfg_r.Gold_csv_L = ext_cfg->rff_mse_low->data;

        SD_CFG->adpt_cfg_r.total_gain_adj_begin = ext_cfg->rff_iir_general->total_gain_freq_l;
        SD_CFG->adpt_cfg_r.total_gain_adj_end = ext_cfg->rff_iir_general->total_gain_freq_h;
        SD_CFG->adpt_cfg_r.gain_limit_all = ext_cfg->rff_iir_general->total_gain_limit;

        // 耳道记忆曲线配置
        SD_CFG->adpt_cfg_r.mem_curve_nums = ext_cfg->rff_ear_mem_param->mem_curve_nums;
        SD_CFG->adpt_cfg_r.sz_table = ext_cfg->rff_ear_mem_sz->data;
        SD_CFG->adpt_cfg_r.pz_table = ext_cfg->rff_ear_mem_pz->data;
        SD_CFG->adpt_cfg_r.pz_table_cmp = NULL;
        SD_CFG->adpt_cfg_r.sz_table_cmp = NULL;
#if EXT_PRINTF_DEBUG
        for (int i = 0; i < 60; i++) {
            printf("idx=%d, mse=%d, weight=%d\n", i, (int)SD_CFG->adpt_cfg.Gold_csv_H[i], (int)SD_CFG->adpt_cfg.Weight_H[i]);
        }

        de_vrange_printf(SD_CFG->adpt_cfg.Vrange_H, SD_CFG->adpt_cfg.IIR_NUM_FLEX + SD_CFG->adpt_cfg.IIR_NUM_FIX);
        de_vrange_printf(SD_CFG->adpt_cfg.Vrange_M, SD_CFG->adpt_cfg.IIR_NUM_FLEX + SD_CFG->adpt_cfg.IIR_NUM_FIX);
        de_vrange_printf(SD_CFG->adpt_cfg.Vrange_L, SD_CFG->adpt_cfg.IIR_NUM_FLEX + SD_CFG->adpt_cfg.IIR_NUM_FIX);

        de_biquad_printf(SD_CFG->adpt_cfg.Biquad_init_H, SD_CFG->adpt_cfg.IIR_NUM_FLEX + SD_CFG->adpt_cfg.IIR_NUM_FIX);
        de_biquad_printf(SD_CFG->adpt_cfg.Biquad_init_M, SD_CFG->adpt_cfg.IIR_NUM_FLEX + SD_CFG->adpt_cfg.IIR_NUM_FIX);
        de_biquad_printf(SD_CFG->adpt_cfg.Biquad_init_L, SD_CFG->adpt_cfg.IIR_NUM_FLEX + SD_CFG->adpt_cfg.IIR_NUM_FIX);
#endif

#endif
    }
}

void icsd_anc_v2_sz_pz_cmp_calculate(struct sz_pz_cmp_cal_param *p)
{
    float eq_freq_l = 50;
    float eq_freq_h = 500;
    u8 mode = 1;	//1 eq gain; 0 eq 频响

#if 0
    printf("ff_gain %d/100\n", (int)(p->ff_gain * 100.0f));
    printf("fb_gain %d/100\n", (int)(p->fb_gain * 100.0f));

    struct aeq_default_seg_tab *eq_tab = p->spk_eq_tab;
    printf("global_gain %d/100\n", (int)(eq_tab->global_gain * 100.f));
    printf("seg_num %d\n", eq_tab->seg_num);
    for (int i = 0; i < eq_tab->seg_num; i++) {
        printf("index %d\n", eq_tab->seg[i].index);
        printf("iir_type %d\n", eq_tab->seg[i].iir_type);
        printf("freq %d\n", eq_tab->seg[i].freq);
        printf("gain %d/100\n", (int)(eq_tab->seg[i].gain * 100.f));
        printf("q %d/100\n", (int)(eq_tab->seg[i].q * 100.f));
    }
#endif
    szpz_cmp_get(p->pz_cmp_out, p->sz_cmp_out, p->ff_gain, p->fb_gain, \
                 (struct aeq_default_seg_tab *)p->spk_eq_tab, eq_freq_l, eq_freq_h, mode);
#if 0
    printf("sz_pz output\n");
    for (int i = 0; i < 50; i++) {
        printf("pz %d/100\n", (int)(p->pz_cmp_out[i] * 100.0f));
        printf("sz %d/100\n", (int)(p->sz_cmp_out[i] * 100.0f));
    }
#endif
}

const float pz_coef_table[] = {
    0.8913, -39.8161, 2952.9700, 0.9997, 3.3358, 1616.6300, 1.0197, -2.0091, 1470.8000, 1.4993, 1.7590, 583.5400, 0.3001, 1.2159, 123.2230, 0.8001,
    0.9156, -39.6423, 3265.5000, 0.9908, 0.9007, 1796.0000, 0.7135, 0.6405, 841.6920, 1.3138, 1.2926, 436.2050, 0.3537, 0.9052, 138.8840, 0.8404,
    0.9198, -36.8223, 2779.3000, 0.9752, 1.8318, 1800.0000, 1.3256, 1.8881, 934.2800, 0.8242, 1.0703, 279.6520, 0.3022, 0.7384, 92.8514, 0.9775,
    0.9503, -34.3658, 2676.5500, 0.9992, 2.2648, 1800.0000, 0.7295, 0.4138, 203.3830, 0.7260, 1.2118, 799.5010, 0.5140, 0.6630, 148.8580, 0.8267,
    0.9815, -40.0000, 2362.5200, 0.8002, -0.8602, 1569.6000, 1.2042, 10.6725, 1267.5400, 0.3544, -1.9402, 482.8200, 0.6852, 0.7110, 70.1474, 0.8030,
    0.9915, -32.5901, 2382.5200, 0.9987, 4.4877, 1799.7300, 0.8515, 1.2886, 999.5240, 1.0768, 0.4730, 280.3400, 0.3000, 0.3683, 70.0699, 0.8744,
    1.0151, -27.2293, 1724.1500, 0.9982, 4.0748, 1799.3900, 1.4986, 5.1767, 1188.5300, 0.8621, 0.1852, 114.1580, 0.3021, 0.5435, 53.5829, 0.8673,
    1.0381, -34.9219, 2825.8000, 0.9997, 2.3813, 1474.2400, 0.4333, 0.9428, 1047.5600, 1.3681, 0.0097, 728.2150, 1.4619, 0.1523, 77.7408, 0.9370,
    1.0525, -37.7446, 3287.1700, 0.9792, -0.8803, 1495.1500, 1.5000, 4.6082, 1391.0700, 0.6829, -0.2129, 798.4630, 0.7025, -0.2137, 57.5778, 0.9938,
    1.0676, -35.0857, 3212.4500, 0.9923, 0.8764, 1144.1700, 1.5000, 2.7029, 1360.5600, 0.5926, -0.1896, 512.0760, 0.7514, -0.3338, 69.4518, 0.8214,
    1.0815, -38.7744, 3700.0000, 0.9073, 5.1765, 1267.9000, 0.7263, -0.7488, 1340.1900, 0.3000, 0.5371, 788.6640, 0.7510, -0.3923, 98.0938, 0.8361,
    1.1121, -34.8591, 2589.8000, 0.9335, 3.4287, 1774.3600, 0.5095, 4.2748, 1270.9800, 0.6792, -0.8153, 526.4180, 0.5516, -0.5343, 142.1010, 0.8103,
    1.1034, -32.4288, 2694.8300, 0.8001, 1.3336, 1800.0000, 0.6681, 6.3189, 1281.7100, 0.8383, 0.6626, 799.9010, 1.4385, -0.6249, 108.6760, 0.8000,
    1.1344, -39.2066, 3609.9400, 0.9996, 2.8054, 1537.6100, 0.5168, 1.6259, 1138.7600, 1.2976, -0.5543, 244.1300, 0.3245, -1.2828, 52.5508, 0.8469,
    1.1326, -39.6885, 3662.0500, 0.8053, 4.4003, 1576.6800, 0.8011, 3.3466, 1144.8400, 1.3331, 1.6878, 730.2320, 1.1713, -1.3798, 58.2555, 0.8001,
    1.1344, -34.3867, 3219.6900, 0.9996, -1.3552, 1436.0200, 1.5000, 5.5128, 1322.9700, 0.9307, -0.4133, 233.7080, 0.3871, -0.6631, 95.9154, 0.8519,
    1.1471, -34.4199, 2746.7400, 0.9534, 4.1818, 1799.9000, 0.8615, 3.5864, 1102.0200, 1.0032, -0.5155, 234.0740, 0.7565, -1.1310, 93.1636, 0.8003,
    1.1314, -30.4768, 2559.2000, 0.9713, -1.6423, 1001.7500, 0.3017, 7.2966, 1357.2200, 0.6748, 0.1179, 787.6640, 0.4900, -1.0815, 61.7469, 0.8119,
    1.1448, -36.8617, 3213.0700, 0.9069, 2.0950, 1794.0000, 0.5642, 4.7935, 1210.6700, 0.8612, -0.5394, 166.6860, 0.3117, -0.8030, 65.0644, 0.8207,
    1.1348, -29.2927, 2554.3600, 1.0000, 6.3162, 1322.4900, 0.8213, -1.2574, 933.9240, 0.3261, 0.3676, 590.5720, 0.7350, -0.6144, 149.9820, 0.8021,
    1.1486, -39.7093, 3699.8200, 0.9341, 4.8017, 1277.3800, 0.6503, 0.9520, 1163.5900, 1.5000, -0.6250, 312.6500, 0.4243, -0.9076, 98.3457, 0.8004,
    1.1529, -33.2132, 2773.2700, 0.9979, 4.1362, 1712.2100, 0.7671, 2.5021, 1137.9500, 1.2350, -0.6215, 238.8100, 0.8032, -1.0167, 103.1050, 0.8732,
    1.1386, -34.6065, 3700.0000, 0.9992, -1.4496, 1570.8600, 1.4981, 5.1271, 1344.0000, 0.9283, -0.4369, 208.1290, 0.3002, -0.8888, 64.0054, 0.8649,
    1.1562, -33.1774, 2903.8500, 0.9866, 4.6856, 1678.4100, 0.6664, 2.0403, 1172.1500, 1.4421, -0.6099, 238.0080, 0.3944, -0.8924, 85.2652, 0.9622,
    1.1628, -31.2519, 2914.5200, 0.9841, 1.5923, 1800.0000, 0.6880, 4.5979, 1280.7800, 1.0238, -0.6019, 171.0850, 0.3058, -0.8311, 71.6556, 0.9393,
    1.1816, -33.3499, 3510.7700, 1.0000, 2.1284, 1287.1400, 1.4999, 2.9913, 1288.3200, 0.8326, -0.6758, 156.4360, 0.3001, -0.8531, 68.9457, 0.9767,
    1.1533, -32.8295, 3327.9400, 0.9273, 4.0724, 1614.8000, 0.5693, 3.2250, 1273.9000, 1.1894, -0.7665, 713.0960, 0.3171, -0.7409, 149.7360, 0.8022,
    1.1551, -28.2117, 2939.2700, 0.9997, 5.5258, 1359.3800, 1.0124, 0.2540, 1499.3900, 0.6039, -0.4964, 279.4500, 0.3000, -0.8528, 87.6227, 0.8937,
    1.1671, -32.5055, 3467.4000, 0.9989, 2.3136, 1292.0100, 1.0026, 2.8138, 1309.7100, 1.1093, -0.5201, 242.2930, 0.5572, -0.8630, 119.7600, 0.9139,
    1.1830, -33.7242, 3037.7200, 0.9764, 5.2167, 1757.9700, 0.6106, 2.8944, 1237.4600, 1.4574, -0.8047, 291.2410, 0.3261, -1.1196, 87.6587, 0.8182,
    1.1605, -30.1534, 2872.3400, 0.9418, 5.3226, 1798.4400, 0.8079, 3.1673, 1218.3000, 1.2329, -0.5288, 222.2470, 0.3222, -1.0409, 69.7973, 0.8272,
    1.1627, -31.5431, 3131.4200, 0.8124, 5.6589, 1533.9200, 0.8549, 3.3226, 1226.0000, 0.9871, -0.5563, 155.1980, 0.3209, -0.9508, 65.4004, 0.8004,
    1.1711, -26.5079, 2449.4500, 1.0000, 5.5416, 1603.7600, 0.9767, 2.7242, 1238.7600, 1.1308, -0.6721, 239.5380, 0.3001, -1.2654, 57.3433, 0.8758,
    1.1697, -29.8757, 2892.4100, 0.9084, 3.3428, 1799.2800, 0.7167, 5.2124, 1350.7400, 1.0833, -0.6228, 196.1640, 0.3482, -1.0190, 73.5294, 0.8000,
    1.1722, -28.0073, 2643.6300, 0.9997, 5.3687, 1799.3600, 0.8541, 3.4983, 1279.9800, 1.4576, -0.6250, 264.7240, 0.4567, -1.0153, 104.9470, 0.9410,
    1.1608, -28.5000, 3023.6100, 0.9938, 2.8012, 1799.7300, 0.6622, 4.7017, 1414.2900, 1.0391, -0.7906, 558.3900, 0.3013, -1.1237, 96.4431, 0.8000,
    1.1601, -27.9416, 3102.2300, 0.9464, -2.4921, 1284.7800, 0.4049, 9.5111, 1471.4300, 0.7914, -0.5219, 129.6490, 0.5403, -1.2152, 55.0359, 0.8944,
    1.1605, -29.8472, 3700.0000, 0.9995, 7.2261, 1405.8100, 0.9456, -1.3785, 1142.7900, 0.9848, -0.5665, 420.8460, 0.5004, -0.8588, 149.9710, 0.8000,
    1.1668, -30.4510, 3646.9400, 0.9543, 4.3741, 1480.8000, 1.1428, 2.9193, 1484.5300, 0.7569, -0.5809, 491.0810, 0.3768, -0.8315, 150.0000, 0.8000,
    1.1668, -30.1339, 3688.5200, 0.9739, 7.0441, 1480.1400, 1.0040, 0.0612, 351.8790, 0.7254, -0.6346, 380.3780, 0.3070, -0.9496, 99.7389, 0.8396,
    1.1588, -28.4857, 3303.4600, 0.9714, 3.9268, 1800.0000, 1.1176, 4.0535, 1391.2100, 1.0627, -0.5322, 248.6790, 0.3403, -0.8848, 89.7204, 0.8531,
    1.1644, -29.2182, 3700.0000, 1.0000, 4.6144, 1484.6300, 1.3116, 2.0911, 1391.3800, 0.7482, -0.5456, 456.1130, 0.3000, -0.7862, 136.2360, 0.8060,
    1.1547, -28.6566, 3700.0000, 0.9553, 7.1950, 1523.0000, 1.0534, -0.0588, 1379.6700, 0.8219, -0.5175, 286.6510, 0.3477, -0.8483, 100.4870, 0.8000,
    1.1597, -27.1052, 3228.1500, 0.9732, 6.0947, 1739.9200, 0.9038, 2.3076, 1405.3800, 1.4999, -0.5578, 279.2870, 0.3023, -1.0073, 75.1964, 0.8324,
    1.1486, -23.0844, 2444.3400, 0.9864, 10.1900, 1678.3700, 0.8804, -0.7166, 932.5050, 1.2987, -0.8459, 497.1890, 0.5661, -0.7421, 149.4720, 0.8000,
    1.1472, -28.2411, 3687.6000, 0.9701, 5.6161, 1788.3700, 0.8965, 2.1687, 1446.9300, 1.4998, -0.5249, 224.8170, 0.3002, -0.9297, 68.8156, 0.8044,
    1.1470, -25.1318, 3259.4100, 0.9969, 4.7462, 1777.8500, 1.1056, 3.0108, 1484.3100, 1.1602, -0.4976, 241.5950, 0.3248, -0.8787, 82.2026, 0.9772,
    1.1478, -25.9535, 3604.8600, 1.0000, 5.8433, 1647.1400, 1.0769, 0.9070, 1414.5200, 1.2439, -0.4480, 282.7360, 0.3647, -0.8043, 100.5530, 0.8014,
    1.1469, -26.7963, 3574.1600, 0.8775, 8.6858, 1650.9900, 0.9633, -0.3354, 1259.6600, 0.6001, -0.4421, 393.7580, 0.5199, -0.7273, 147.1600, 0.8130,
    1.1523, -26.9736, 3700.0000, 1.0000, 7.7405, 1696.9300, 1.0736, 0.1002, 1356.8800, 1.4938, -0.4878, 384.5470, 0.3688, -0.7225, 121.7260, 0.9490,
    1.1399, -22.2477, 2195.9100, 0.9455, 15.5387, 1766.0300, 0.6882, -4.0855, 1065.5200, 0.5209, -0.4564, 532.3520, 0.3027, -0.5714, 149.9150, 0.9975,
    1.1377, -23.7575, 3147.8700, 0.9333, 9.4828, 1797.4800, 0.9233, 0.3479, 446.1850, 0.6505, -1.0825, 700.7520, 0.3000, -0.6348, 125.4800, 0.9811,
    1.1367, -24.8132, 3700.0000, 0.9944, 7.1060, 1800.0000, 1.0520, 0.3936, 1476.5800, 1.2997, -0.4220, 320.4970, 0.3000, -0.6633, 101.4400, 0.9142,
    1.1411, -24.9958, 3699.9000, 1.0000, 8.0496, 1778.7300, 1.0680, -0.0024, 1131.2100, 1.0806, -0.4641, 533.0070, 0.3405, -0.7268, 121.8910, 0.8018,
    1.1370, -20.0089, 2605.8500, 0.9990, 11.0635, 1800.0000, 1.1366, -0.4923, 1194.3300, 0.8165, -0.5006, 524.3590, 0.5445, -0.5776, 149.8190, 0.8674,
    1.1310, -20.5848, 2771.0900, 0.8880, 10.4643, 1799.6100, 0.9384, -1.0986, 1280.5300, 0.6335, -0.3779, 283.2450, 0.3021, -0.7083, 79.9311, 0.8250,
    1.1289, -21.7110, 3697.2400, 0.9861, 6.6406, 1800.0000, 1.2384, 0.5092, 1123.9300, 0.9531, -0.3400, 424.1030, 0.4885, -0.6034, 142.8880, 0.8001,
    1.1280, -20.9955, 3700.0000, 1.0000, 7.4417, 1799.9900, 1.0920, -0.8342, 1312.4700, 0.3004, 0.4004, 799.6460, 1.1690, -0.5005, 149.6980, 0.9920,
    1.1334, -22.2811, 3698.8800, 0.9150, 8.3898, 1800.0000, 1.2837, 0.3038, 987.1720, 1.0323, -0.3197, 395.4180, 0.4852, -0.5350, 148.5900, 0.9040,
    1.1264, -20.7150, 3700.0000, 0.9863, 6.8790, 1799.3100, 1.2542, 0.4020, 969.0210, 1.1573, -0.3033, 476.5470, 0.3768, -0.4894, 149.9710, 0.8446,
};
const float sz_coef_table[] = {
    1.8413, -39.9909, 1517.6000, 0.8259, 3.4567, 1747.5600, 0.6954, 3.2044, 200.0010, 0.3037, 8.1049, 800.0000, 0.4537, -7.5769, 53.9404, 1.0000,
    1.7212, -39.9990, 2633.4100, 0.9983, -2.5179, 1798.5700, 0.6825, 2.1906, 1337.1700, 0.9141, 4.4067, 225.7010, 0.3000, -7.5769, 63.6655, 1.0000,
    1.7149, -39.9976, 2120.7000, 0.8672, 0.0139, 1759.7800, 1.5000, -4.2115, 399.5460, 0.6767, 10.0402, 478.4350, 0.3008, -5.6421, 60.3684, 0.9995,
    1.6298, -39.9998, 2302.7700, 0.8943, 0.8485, 1648.1400, 1.5000, 2.7239, 1068.6100, 0.9456, 4.5513, 300.8500, 0.3006, -7.8614, 58.4026, 0.9975,
    1.5830, -38.8801, 2301.5300, 0.9767, 0.1925, 1798.2900, 1.5000, 2.9884, 1168.5300, 0.8160, 3.3837, 213.5960, 0.3368, -8.2724, 63.8358, 0.9996,
    1.5110, -39.9796, 2250.6800, 0.8781, 2.8426, 1349.0000, 0.8324, 3.0935, 200.0000, 0.5701, 3.6385, 799.4660, 0.5146, -10.0000, 50.8707, 1.0000,
    1.4730, -40.0000, 2979.4300, 0.9995, -0.9696, 1787.6200, 1.4904, 0.7600, 201.7280, 0.7395, 2.5577, 250.8680, 0.3045, -9.9540, 60.2485, 0.9999,
    1.5363, -40.0000, 2311.6200, 0.8851, 1.0548, 1715.2700, 1.4332, 4.2556, 1087.8200, 0.8972, 2.9750, 330.6170, 0.3000, -9.9254, 65.7642, 1.0000,
    1.3902, -40.0000, 2641.2500, 0.9201, 3.7191, 1180.8400, 0.9271, -0.4386, 509.0610, 0.4805, 2.9119, 296.4740, 0.3622, -9.9704, 63.2030, 1.0000,
    1.2953, -38.4125, 2466.5700, 0.9354, 4.3201, 1311.9400, 0.9298, 2.2230, 201.3870, 0.6305, 1.0048, 688.8360, 0.5190, -9.9972, 65.6087, 0.9963,
    1.3162, -39.0290, 2759.0600, 0.9409, 3.5431, 1176.4600, 1.1068, 2.1186, 200.0000, 0.5185, 0.5053, 592.0530, 1.1707, -9.9673, 74.8296, 0.9557,
    1.2719, -36.9875, 2422.1000, 0.9456, 1.3944, 1256.1800, 0.8008, 3.4348, 1221.2000, 0.8271, 1.3099, 243.7480, 0.6610, -9.9953, 75.4513, 0.9960,
    1.1423, -37.4125, 2667.7000, 0.9413, 1.8015, 1247.9400, 1.4894, 2.8711, 1242.3800, 0.7494, 1.5836, 193.2760, 0.5333, -8.6415, 86.3258, 1.0000,
    1.2245, -38.4695, 2968.3200, 0.9998, -1.5280, 1538.9000, 0.4676, 3.4966, 1222.8200, 1.1050, 0.9840, 468.3280, 0.3096, -9.9777, 90.8132, 0.9995,
    1.2043, -36.0290, 2531.0300, 0.9770, 1.7478, 1354.3300, 0.7828, 2.6501, 1208.6100, 1.3906, 0.7119, 298.5570, 0.4818, -9.9982, 91.4251, 0.9999,
    1.1969, -33.5520, 1968.1000, 0.8682, 4.8349, 1259.4900, 0.7350, 2.5261, 1274.2200, 1.0567, 1.0215, 407.2140, 0.3781, -9.9931, 93.1328, 0.9995,
    1.2026, -32.5944, 1818.1000, 0.9382, 3.1307, 1524.1100, 0.9224, 5.0980, 1162.5100, 0.8253, 0.8811, 207.4130, 0.5400, -9.9970, 97.3320, 0.9881,
    1.1452, -36.9649, 2706.8800, 0.9766, 3.7991, 1233.9400, 1.1021, 0.1024, 1231.9000, 0.4080, 1.0873, 201.4860, 0.6155, -9.9964, 91.4164, 0.9994,
    1.1670, -32.6614, 2208.0700, 0.9496, 4.3352, 1281.0500, 1.1201, 0.6477, 1116.5800, 1.2103, 0.9085, 492.2020, 0.3059, -9.9993, 95.9251, 0.9993,
    1.1354, -39.0042, 2639.0900, 0.8093, 5.7951, 1199.4400, 0.9076, 0.0979, 842.5880, 0.8502, 1.8730, 629.6220, 0.3126, -9.9979, 91.3786, 0.9998,
    1.1816, -38.9893, 2865.0900, 0.8526, -2.0801, 1800.0000, 0.8038, 6.8523, 1253.9100, 0.8414, 1.0747, 627.0140, 0.3716, -9.9979, 100.2300, 0.9972,
    1.1233, -33.8914, 2891.0200, 0.8882, -9.6172, 1768.5500, 0.5069, 12.2712, 1320.7400, 0.6708, 0.5350, 212.5400, 0.6063, -10.0000, 104.2440, 1.0000,
    1.0033, -36.5523, 2584.5200, 0.8015, 5.9914, 1276.0700, 0.8969, 0.6005, 1278.8700, 1.2013, 1.4039, 547.3150, 0.3046, -9.9818, 95.4370, 0.9997,
    1.0313, -32.8779, 1998.4900, 0.8858, 5.2492, 1274.8100, 1.0387, 3.0512, 1479.2100, 0.4021, 0.6135, 156.4730, 1.1476, -10.0000, 104.1500, 0.9998,
    0.9508, -32.2533, 2639.2800, 0.9386, -4.3345, 1773.8200, 1.0490, 8.1262, 1406.5700, 0.9780, 0.3395, 348.2000, 1.0280, -9.9880, 110.0930, 0.9997,
    0.9233, -29.9156, 2546.4700, 0.9485, -0.0655, 1501.0400, 0.7477, 4.3688, 1273.3800, 1.3997, 0.8792, 721.0130, 0.3132, -9.9999, 130.8040, 1.0000,
    0.8551, -29.9512, 2610.2200, 0.9796, -3.4076, 1798.6100, 1.3936, 6.6896, 1455.8000, 1.1382, 0.8116, 193.5610, 1.2124, -9.9925, 115.0770, 0.9991,
    0.8240, -26.5365, 1926.8800, 0.9819, -2.1776, 1740.1300, 0.9264, 9.4825, 1427.1400, 0.9844, -0.6247, 705.0970, 1.4685, -9.9990, 107.3890, 0.9989,
    0.8466, -25.0413, 2459.0800, 0.9591, -10.0454, 1788.3100, 0.6574, 12.7339, 1411.0900, 0.8897, 1.0360, 208.8780, 1.2933, -9.9979, 130.8070, 0.9976,
    0.8443, -36.3616, 3215.5600, 0.8940, -2.3210, 1772.2400, 1.4929, 7.4220, 1369.7500, 0.8379, -3.8910, 100.0850, 1.0777, -9.9929, 104.4260, 0.9998,
    0.7730, -31.5284, 2971.1900, 0.9645, 5.3346, 1344.0800, 1.2549, -0.7579, 1399.7800, 0.7454, 0.4827, 396.6680, 0.5304, -9.9956, 113.1570, 0.9993,
    0.7439, -26.4113, 2145.6100, 0.9912, 0.8058, 1191.9800, 1.3745, 6.1391, 1432.6200, 1.1420, 0.4095, 187.9290, 1.3858, -9.9917, 119.5720, 0.9995,
    0.7491, -25.7476, 2131.8200, 0.9533, 6.3573, 1345.9400, 1.1906, 0.2660, 1386.2200, 1.3454, 0.3820, 793.6700, 0.3000, -9.9988, 124.6220, 1.0000,
    0.7116, -23.2892, 1648.1100, 0.9980, 1.3134, 1286.1700, 1.3547, 8.1993, 1433.8900, 1.0257, -3.6607, 100.0290, 1.0520, -9.9941, 102.0740, 0.9983,
    0.6848, -31.6026, 3089.8500, 0.8206, 1.8355, 1003.2400, 0.3001, 5.2494, 1272.9200, 1.4751, -4.5416, 100.0690, 0.8758, -9.9775, 99.7699, 0.9999,
    0.6323, -25.2715, 2043.8700, 0.9576, 0.5149, 1702.2100, 0.4840, 7.9379, 1477.6600, 1.0603, -2.5910, 100.0000, 0.9008, -9.9977, 103.0310, 0.9971,
    0.5936, -26.5214, 2451.7000, 0.9912, 6.7699, 1492.2200, 1.0977, 0.1730, 1347.7500, 0.9102, -3.5729, 100.0050, 0.9997, -9.9912, 99.6963, 0.9997,
    0.5505, -27.1011, 2705.6800, 0.9548, 3.6580, 1456.0000, 1.3756, 3.0202, 1480.5900, 0.6705, -3.9955, 100.0130, 1.0105, -9.9995, 100.2860, 0.9999,
    0.5393, -31.3311, 3576.4500, 0.9830, -5.0887, 1668.8700, 1.0570, 10.0974, 1500.0000, 1.0165, -4.6807, 100.0650, 0.9960, -9.9992, 100.2350, 0.9993,
    0.5288, -29.6074, 3267.7900, 0.8176, -8.1423, 1670.2000, 0.8022, 15.3446, 1487.8500, 0.9008, -4.5698, 100.0640, 0.9519, -9.9998, 106.7460, 0.9991,
    0.5225, -25.1424, 2573.3400, 0.9436, 6.6132, 1493.9400, 1.0922, 0.6983, 382.1290, 0.3204, -3.8311, 100.0470, 0.7549, -9.9955, 105.6890, 0.9999,
    0.4982, -25.1871, 2943.5000, 0.9674, 5.4216, 1387.9800, 1.2209, 0.8275, 315.8890, 0.3214, -6.2371, 100.0000, 0.9978, -9.9970, 104.3900, 0.9997,
    0.4394, -21.5580, 2735.7200, 0.9878, 5.7060, 1431.5700, 1.3652, 1.3314, 300.6460, 0.3000, -7.4546, 100.0170, 0.8843, -9.9872, 99.1620, 1.0000,
    0.4288, -23.5145, 2729.4100, 0.8696, 5.6379, 1472.4100, 1.4347, 2.2383, 1259.0300, 0.3005, -6.0193, 100.0200, 0.9717, -9.9867, 102.1370, 0.9983,
    0.3911, -22.2415, 1500.0000, 0.8881, 12.4435, 1799.9800, 0.6508, 3.0570, 1417.0800, 1.4646, -7.1864, 100.0000, 1.1513, -9.9997, 100.2990, 0.9972,
    0.3740, -22.0816, 2854.0600, 0.9750, 6.2911, 1525.9800, 0.9032, 6.9796, 200.9450, 0.4877, -11.4165, 100.0530, 0.3520, -9.9843, 88.2492, 0.9996,
    0.3708, -23.5233, 3303.1400, 0.9095, 6.0640, 1491.3300, 0.9419, 2.8783, 200.1460, 0.5880, -8.7639, 100.0010, 0.7146, -9.9981, 100.2400, 1.0000,
    0.3663, -19.6017, 2074.5800, 0.8001, 9.2552, 1583.0000, 1.1377, 1.6139, 585.4610, 0.3055, -7.7545, 100.0100, 0.9490, -9.9954, 102.2340, 0.9990,
    0.3399, -19.1375, 2565.7800, 0.8044, 6.1743, 1514.0300, 1.4159, 2.1995, 1081.2500, 0.3020, -8.1365, 100.0350, 1.1189, -9.9980, 97.2797, 0.9993,
    0.3226, -24.9271, 2254.1000, 0.8087, 13.6074, 1655.8500, 0.5641, -2.3241, 894.5000, 0.9640, -9.9281, 100.0710, 1.2630, -10.0000, 97.2956, 0.9972,
    0.3013, -18.3345, 2030.7500, 0.8405, 8.3021, 1680.7600, 1.3945, 2.8006, 1270.1900, 0.3085, -7.7253, 100.0060, 1.1114, -9.9712, 102.3340, 1.0000,
    0.2803, -16.5898, 3103.8600, 0.8013, 5.7512, 1476.5500, 1.4856, 2.3053, 392.3810, 0.3016, -9.2936, 100.0480, 0.8173, -9.9942, 91.4150, 0.9999,
    0.2603, -16.9485, 2857.4600, 0.9384, 6.8249, 1614.0000, 1.0341, 2.7091, 261.5410, 0.4023, -8.9837, 100.0860, 0.7233, -9.9945, 97.3756, 0.9981,
    0.2394, -15.9474, 2484.2600, 0.9886, 8.0993, 1662.8300, 1.1282, 2.0379, 315.6820, 0.3056, -12.9338, 100.0670, 1.1852, -9.9924, 97.3388, 1.0000,
    0.2297, -14.5461, 3630.1600, 0.9500, 5.2704, 1439.5100, 1.2489, 2.6414, 339.8960, 0.3025, -13.3416, 100.0910, 1.1175, -9.9976, 97.3378, 0.9998,
    0.2295, -16.5010, 3391.9900, 0.8405, 4.9466, 1589.9500, 1.4384, 2.5147, 820.3960, 0.3000, -9.5214, 100.0190, 1.2063, -9.9979, 100.2700, 0.9993,
    0.2161, -13.4792, 2861.4600, 0.9931, 5.2676, 1635.7900, 1.2282, 2.4310, 442.3380, 0.3004, -11.0020, 100.0570, 1.0185, -9.9988, 94.7897, 0.9999,
    0.1997, -14.9858, 3381.1600, 0.9807, 5.3825, 1615.8900, 0.5743, 3.8110, 213.2510, 0.5245, -10.2446, 100.0130, 0.6232, -9.9992, 100.2830, 0.9996,
    0.1933, -15.8559, 3325.1900, 0.9314, 6.9656, 1741.5900, 0.8719, 2.8657, 285.1890, 0.3174, -14.0457, 100.0120, 1.0952, -9.9994, 97.3879, 0.9999,
    0.1936, -12.0416, 2327.5400, 0.8012, 7.4718, 1667.3400, 1.2013, 2.7536, 454.7670, 0.3070, -10.4579, 100.0400, 0.9337, -9.9963, 100.2950, 0.9994,
};

#endif
