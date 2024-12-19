#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".icsd_cmp.data.bss")
#pragma data_seg(".icsd_cmp.data")
#pragma const_seg(".icsd_cmp.text.const")
#pragma code_seg(".icsd_cmp.text")
#endif

#include "app_config.h"
#include "audio_config_def.h"
#if (TCFG_AUDIO_ANC_EAR_ADAPTIVE_EN && \
	 TCFG_AUDIO_ANC_ENABLE && \
	 TCFG_AUDIO_ANC_EAR_ADAPTIVE_VERSION == ANC_EXT_V2)

#include "audio_anc.h"

#if ANC_EAR_ADAPTIVE_CMP_EN

#include "icsd_cmp.h"
#include "icsd_cmp_app.h"
#include "a2dp_player.h"
#include "esco_player.h"

#if 1
#define cmp_log printf
#else
#define cmp_log (...)
#endif

#define ANC_ADAPTIVE_CMP_RUN_TIME_DEBUG			0		//CMP 算法运行时间测试

struct icsd_cmp_param {
    _cmp_output *output;					//算法输出
    float gain;								//增益
    anc_fr_t fr[ANC_ADAPTIVE_CMP_ORDER];	//输出整理
    double *coeff;							//IIR_COEFF
};

struct icsd_cmp_hdl {
    u8 state;
    u8 busy;
    void *lib_alloc_ptr;

    struct icsd_cmp_param l_param;
#if ANC_CONFIG_RFB_EN
    struct icsd_cmp_param r_param;
#endif
};

struct icsd_cmp_hdl *cmp_hdl = NULL;

static int audio_anc_ear_adaptive_cmp_close(void);

void icsd_cmp_force_exit()
{
    if (cmp_hdl) {
        DeAlorithm_disable();
    }
}

int audio_anc_ear_adaptive_cmp_run(__afq_output *p, u8 data_from)
{
    int ret = 0;
    if (!cmp_hdl) {
        return -1;
    }
#if (ANC_ADAPTIVE_CMP_ONLY_IN_MUSIC_UPDATE && TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE)
    //非通话/播歌状态 不更新
    if (!(a2dp_player_runing() || esco_player_runing())) {
        return 0;
    }
#endif
    if (cmp_hdl->state == ANC_EAR_ADAPTIVE_CMP_STATE_CLOSE) {
        return -1;
    }
    cmp_hdl->busy = 1;
    cmp_log("ICSD_CMP_STATE:RUN");
    u8 l_state = 0, r_state = 0;
    s8 sz_sign = anc_ext_ear_adaptive_sz_calr_sign_get();
    cmp_log("sz_sign %d\n", sz_sign);
    float *sz_l_tmp, sz_r_tmp;
    if (p->sz_l) {
#if ANC_ADAPTIVE_CMP_RUN_TIME_DEBUG
        u32 last = jiffies_usec();
#endif
        //CMP 算法要求输入SZ的符号必须为正，因此需要单独备份SZ的数据，以便修改
        sz_l_tmp = (float *)zalloc(p->sz_l->len * sizeof(float));
        memcpy((u8 *)sz_l_tmp, (u8 *)p->sz_l->out, p->sz_l->len * sizeof(float));
        //CMP来自实时自适应时，SZ符合一定为正，不需要翻转符号
        if ((sz_sign == -1) && (data_from != CMP_FROM_RTANC)) {
            for (int j = 0; j < p->sz_l->len; j++) {
                sz_l_tmp[j] = 0 -  sz_l_tmp[j];
            }
        }
        cmp_hdl->l_param.output = icsd_cmp_run(sz_l_tmp);
        l_state = cmp_hdl->l_param.output->state;
        //CMP增益符号与SZ符号 负相关
        cmp_hdl->l_param.output->fgq[0] = (abs(cmp_hdl->l_param.output->fgq[0])) * (sz_sign) * (-1);

#if ANC_ADAPTIVE_CMP_RUN_TIME_DEBUG
        cmp_log("ANC ADAPTIVE CMP RUN time: %d us\n", (int)(jiffies_usec2offset(last, jiffies_usec())));
#endif

#if 0	//CMP结果输出打印
        float *fgq = cmp_hdl->l_param.output->fgq;
        g_printf("Global %d/10000\n", (int)(fgq[0] * 10000));
        for (int i = 0; i < ANC_ADAPTIVE_CMP_ORDER; i++) {
            g_printf("SEG[%d]:Type %d, FGQ/10000:%d %d %d\n", i, icsd_cmp_type[i], \
                     (int)(fgq[i * 3 + 1] * 10000), (int)(fgq[i * 3 + 2] * 10000), \
                     (int)(fgq[i * 3 + 3] * 10000));
        }
#endif
        free(sz_l_tmp);
    }
#if ANC_CONFIG_RFB_EN
    if (p->sz_r) {
        sz_r_tmp = (float *)zalloc(p->sz_r->len * sizeof(float));
        memcpy((u8 *)sz_r_tmp, (u8 *)p->sz_r->out, p->sz_r->len * sizeof(float));
        if (sz_sign == -1) {
            for (int j = 0; j < p->sz_r->len; j++) {
                sz_r_tmp[j] = 0 -  sz_r_tmp[j];
            }
        }
        cmp_hdl->r_param.output = icsd_cmp_run(sz_r_tmp);
        r_state = cmp_hdl->r_param.output->state;
        cmp_hdl->r_param.output->fgq[0] = (abs(cmp_hdl->r_param.output->fgq[0])) * (sz_sign) * (-1);
        free(sz_r_tmp);
    }
#endif
    if (l_state || r_state) {
        cmp_log("CMP OUTPUT ERR!! STATE : L %d, R %d\n", l_state, r_state);
        ret = -1;
    }
    cmp_hdl->busy = 0;
    cmp_log("ICSD_CMP_STATE:RUN FINISH");
    return ret;
}

int audio_anc_ear_adaptive_cmp_open(void)
{
    if (cmp_hdl) {
        return -1;
    }
    cmp_hdl = zalloc(sizeof(struct icsd_cmp_hdl));
    enum audio_adaptive_fre_sel fre_sel = AUDIO_ADAPTIVE_FRE_SEL_ANC;

    struct icsd_cmp_libfmt libfmt;
    struct icsd_cmp_infmt  fmt;
    cmp_log("ICSD_CMP_STATE:OPEN");
    cmp_hdl->state = ANC_EAR_ADAPTIVE_CMP_STATE_OPEN;
    icsd_cmp_get_libfmt(&libfmt);
    mem_stats();
    cmp_log("CMP RAM SIZE:%d %d %d\n", libfmt.lib_alloc_size, cmp_IIR_NUM_FIX, cmp_IIR_NUM_FLEX);
    cmp_hdl->lib_alloc_ptr = fmt.alloc_ptr = zalloc(libfmt.lib_alloc_size);
    icsd_cmp_set_infmt(&fmt);
    return 0;
}

int audio_anc_ear_adaptive_cmp_close(void)
{
    if (cmp_hdl) {
        cmp_log("ICSD_CMP_STATE:CLOSE");
        cmp_hdl->state = ANC_EAR_ADAPTIVE_CMP_STATE_CLOSE;
        icsd_cmp_force_exit();
        u8 wait_cnt = 10;
        while (cmp_hdl->busy && wait_cnt) {
            os_time_dly(1);
            wait_cnt--;
        }
        if (cmp_hdl->l_param.coeff) {
            free(cmp_hdl->l_param.coeff);
        }
#if ANC_CONFIG_RFB_EN
        if (cmp_hdl->r_param.coeff) {
            free(cmp_hdl->r_param.coeff);
        }
#endif
        free(cmp_hdl->lib_alloc_ptr);
        free(cmp_hdl);
        cmp_hdl = NULL;
        cmp_log("ICSD_CMP_STATE:CLOSE FINISH");
    }
    return 0;
}

struct icsd_cmp_hdl *audio_anc_ear_adaptive_cmp_hdl_get(void)
{
    return cmp_hdl;
}

//获取算法输出
float *audio_anc_ear_adaptive_cmp_output_get(enum ANC_EAR_ADAPTIVE_CMP_CH ch)
{
    if (cmp_hdl) {
        switch (ch) {
        case ANC_EAR_ADAPTIVE_CMP_CH_L:
            if (cmp_hdl->l_param.output) {
                return cmp_hdl->l_param.output->fgq;
            }
            break;
        case ANC_EAR_ADAPTIVE_CMP_CH_R:
#if ANC_CONFIG_RFB_EN
            if (cmp_hdl->r_param.output) {
                return cmp_hdl->r_param.output->fgq;
            }
#endif
            break;
        }
    }
    return NULL;
}

static void audio_rtanc_adaptive_cmp_data_format(struct icsd_cmp_param *p)
{
    u8 *cmp_dat = (u8 *)&p->gain;
    audio_anc_fr_format(cmp_dat, p->output->fgq, ANC_ADAPTIVE_CMP_ORDER, icsd_cmp_type);
    if (p->coeff == NULL) {
        p->coeff = malloc(ANC_ADAPTIVE_CMP_ORDER * sizeof(double) * 5);
    }
    int alogm = audio_anc_gains_alogm_get(ANC_CMP_TYPE);
    audio_anc_biquad2ab_double(p->fr, p->coeff, ANC_ADAPTIVE_CMP_ORDER, alogm);
}

//获取Total gain & IIR COEFF
int audio_rtanc_adaptive_cmp_output_get(struct anc_cmp_param_output *output)
{
    if (cmp_hdl) {
        audio_rtanc_adaptive_cmp_data_format(&cmp_hdl->l_param);
        output->l_gain = (cmp_hdl->l_param.gain < 0) ? (0 - cmp_hdl->l_param.gain) : cmp_hdl->l_param.gain;
        output->l_coeff = cmp_hdl->l_param.coeff;
#if ANC_CONFIG_RFB_EN
        audio_rtanc_adaptive_cmp_data_format(&cmp_hdl->r_param);
        output->r_gain = (cmp_hdl->r_param.gain < 0) ? (0 - cmp_hdl->r_param.gain) : cmp_hdl->r_param.gain;
        output->r_coeff = cmp_hdl->r_param.coeff;
#endif
    }
    return 0;
}

u8 audio_anc_ear_adaptive_cmp_result_get(void)
{
    u8 result = 0;
    if (cmp_hdl) {
        if (!cmp_hdl->l_param.output->state) {
            result |= ANC_ADAPTIVE_RESULT_LCMP;
        }
#if ANC_CONFIG_RFB_EN
        if (!cmp_hdl->r_param.output->state) {
            result |= ANC_ADAPTIVE_RESULT_RCMP;
        }
#endif
    }
    return result;
}

#endif
#endif


