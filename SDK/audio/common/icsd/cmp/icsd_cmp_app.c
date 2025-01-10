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

#define ANC_ADAPTIVE_CMP_OUTPUT_SIZE		((1 + 3 * ANC_ADAPTIVE_CMP_ORDER) * sizeof(float))

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

void icsd_cmp_force_exit(void)
{
    if (cmp_hdl) {
        DeAlorithm_disable();
    }
}

static int anc_ear_adaptive_cmp_run_ch(struct icsd_cmp_param *p, __afq_data *sz, u8 data_from)
{
    float *sz_tmp;
    _cmp_output *output;
    s8 sz_sign = anc_ext_ear_adaptive_sz_calr_sign_get();
    cmp_log("sz_sign %d\n", sz_sign);
    if (sz) {
#if ANC_ADAPTIVE_CMP_RUN_TIME_DEBUG
        u32 last = jiffies_usec();
#endif
        //CMP 算法要求输入SZ的符号必须为正，因此需要单独备份SZ的数据，以便修改
        sz_tmp = (float *)zalloc(sz->len * sizeof(float));
        memcpy((u8 *)sz_tmp, (u8 *)sz->out, sz->len * sizeof(float));
        //CMP来自实时自适应时，SZ符合一定为正，不需要翻转符号
        if ((sz_sign == -1) && (data_from != CMP_FROM_RTANC)) {
            for (int j = 0; j < sz->len; j++) {
                sz_tmp[j] = 0 -  sz_tmp[j];
            }
        }
        output = icsd_cmp_run(sz_tmp);

        g_printf("ANC_ADAPTIVE_CMP_OUTPUT_SIZE %d\n", ANC_ADAPTIVE_CMP_OUTPUT_SIZE);

        p->output->state = output->state;
        memcpy((u8 *)p->output->fgq, (u8 *)output->fgq, ANC_ADAPTIVE_CMP_OUTPUT_SIZE);

        //CMP增益符号与SZ符号 负相关
        p->output->fgq[0] = (abs(p->output->fgq[0])) * (sz_sign) * (-1);

#if ANC_ADAPTIVE_CMP_RUN_TIME_DEBUG
        cmp_log("ANC ADAPTIVE CMP RUN time: %d us\n", (int)(jiffies_usec2offset(last, jiffies_usec())));
#endif

#if 0	//CMP结果输出打印
        float *fgq = p->output->fgq;
        g_printf("Global %d/10000\n", (int)(fgq[0] * 10000));
        for (int i = 0; i < ANC_ADAPTIVE_CMP_ORDER; i++) {
            g_printf("SEG[%d]:Type %d, FGQ/10000:%d %d %d\n", i, icsd_cmp_type[i], \
                     (int)(fgq[i * 3 + 1] * 10000), (int)(fgq[i * 3 + 2] * 10000), \
                     (int)(fgq[i * 3 + 3] * 10000));
        }
#endif

        free(sz_tmp);
    }
    return p->output->state;
}

int audio_anc_ear_adaptive_cmp_run(__afq_output *p, u8 data_from)
{
    int ret = 0;
    u8 l_state = 0, r_state = 0;
    struct icsd_cmp_libfmt libfmt;
    struct icsd_cmp_infmt  fmt;

    if (!cmp_hdl) {
        return -1;
    }
#if (ANC_ADAPTIVE_CMP_ONLY_IN_MUSIC_UPDATE && TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE)
    //非通话/播歌状态 不更新
    if (!(a2dp_player_runing() || esco_player_runing())) {
        return 0;
    }
#endif
    if ((cmp_hdl->state == ANC_EAR_ADAPTIVE_CMP_STATE_CLOSE) || cmp_hdl->busy) {
        return -1;
    }
    cmp_hdl->busy = 1;
    cmp_log("ICSD_CMP_STATE:RUN");

    //申请算法资源
    icsd_cmp_get_libfmt(&libfmt);
    cmp_log("CMP RAM SIZE:%d %d %d\n", libfmt.lib_alloc_size, cmp_IIR_NUM_FIX, cmp_IIR_NUM_FLEX);
    cmp_hdl->lib_alloc_ptr = fmt.alloc_ptr = zalloc(libfmt.lib_alloc_size);
    g_printf("%p--%x \n", cmp_hdl->lib_alloc_ptr, ((u32)cmp_hdl->lib_alloc_ptr) + libfmt.lib_alloc_size);
    icsd_cmp_set_infmt(&fmt);

    l_state = anc_ear_adaptive_cmp_run_ch(&cmp_hdl->l_param, p->sz_l, data_from);
#if ANC_CONFIG_RFB_EN
    r_state = anc_ear_adaptive_cmp_run_ch(&cmp_hdl->r_param, p->sz_r, data_from);
#endif
    if (l_state || r_state) {
        cmp_log("CMP OUTPUT ERR!! STATE : L %d, R %d\n", l_state, r_state);
        ret = -1;
    }
    cmp_hdl->busy = 0;

    //释放算法资源
    free(cmp_hdl->lib_alloc_ptr);

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

    cmp_log("ICSD_CMP_STATE:OPEN");
    cmp_hdl->state = ANC_EAR_ADAPTIVE_CMP_STATE_OPEN;
    cmp_hdl->l_param.output = malloc(sizeof(_cmp_output));
    cmp_hdl->l_param.output->fgq = malloc(ANC_ADAPTIVE_CMP_OUTPUT_SIZE);
    g_printf("%p %p\n", cmp_hdl->l_param.output, cmp_hdl->l_param.output->fgq);

#if ANC_CONFIG_RFB_EN
    cmp_hdl->r_param.output = malloc(sizeof(_cmp_output));
    cmp_hdl->r_param.output->fgq = malloc(ANC_ADAPTIVE_CMP_OUTPUT_SIZE);
#endif
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
        if (cmp_hdl->l_param.output) {
            free(cmp_hdl->l_param.output);
            free(cmp_hdl->l_param.output->fgq);
        }
#if ANC_CONFIG_RFB_EN
        if (cmp_hdl->r_param.coeff) {
            free(cmp_hdl->r_param.coeff);
        }
        if (cmp_hdl->r_param.output) {
            free(cmp_hdl->r_param.output);
            free(cmp_hdl->r_param.output->fgq);
        }
#endif
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

static void audio_rtanc_adaptive_cmp_data_format(struct icsd_cmp_param *p, double *coeff)
{
    u8 *cmp_dat = (u8 *)&p->gain;
    audio_anc_fr_format(cmp_dat, p->output->fgq, ANC_ADAPTIVE_CMP_ORDER, icsd_cmp_type);
    /* if (p->coeff == NULL) { */
    /* p->coeff = malloc(ANC_ADAPTIVE_CMP_ORDER * sizeof(double) * 5); */
    /* } */
    int alogm = audio_anc_gains_alogm_get(ANC_CMP_TYPE);
    audio_anc_biquad2ab_double(p->fr, coeff, ANC_ADAPTIVE_CMP_ORDER, alogm);
}

//获取Total gain & IIR COEFF, 外部传参准备好coeff 空间
int audio_rtanc_adaptive_cmp_output_get(struct anc_cmp_param_output *output)
{
    if (cmp_hdl) {
        audio_rtanc_adaptive_cmp_data_format(&cmp_hdl->l_param, output->l_coeff);
        output->l_gain = (cmp_hdl->l_param.gain < 0) ? (0 - cmp_hdl->l_param.gain) : cmp_hdl->l_param.gain;
        /* output->l_coeff = cmp_hdl->l_param.coeff; */
#if ANC_CONFIG_RFB_EN
        audio_rtanc_adaptive_cmp_data_format(&cmp_hdl->r_param, output->r_coeff);
        output->r_gain = (cmp_hdl->r_param.gain < 0) ? (0 - cmp_hdl->r_param.gain) : cmp_hdl->r_param.gain;
        /* output->r_coeff = cmp_hdl->r_param.coeff; */
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


