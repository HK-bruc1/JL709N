#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rt_anc.data.bss")
#pragma data_seg(".rt_anc.data")
#pragma const_seg(".rt_anc.text.const")
#pragma code_seg(".rt_anc.text")
#endif

#include "app_config.h"
#include "audio_config_def.h"

#if TCFG_AUDIO_ANC_ENABLE && TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE
#include "audio_anc.h"
#include "rt_anc.h"
#include "rt_anc_app.h"
#include "clock_manager/clock_manager.h"
#include "icsd_anc_user.h"

#include "icsd_adt.h"
#include "icsd_adt_app.h"

#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif/*TCFG_USER_TWS_ENABLE*/

#if TCFG_AUDIO_FREQUENCY_GET_ENABLE
#include "icsd_afq_app.h"
#endif

#if ANC_EAR_ADAPTIVE_CMP_EN
#include "icsd_cmp_app.h"
#endif

#if TCFG_AUDIO_ADAPTIVE_EQ_ENABLE
#include "icsd_aeq_app.h"
#endif

#if 1
#define rtanc_log	printf
#else
#define rtanc_log(...)
#endif/*log_en*/

struct audio_rt_anc_hdl {
    u8 seq;
    u8 state;
    u8 run_busy;
    u8 reset_busy;
    u8 app_func_en;
    u8 lff_iir_type[10];
    u8 rff_iir_type[10];
    u8 fade_gain_suspend;
    int rtanc_mode;
    int suspend_cnt;
    audio_anc_t *param;
    float *debug_param;
    struct icsd_rtanc_tool_data *rtanc_tool;	//工具临时数据
    struct rt_anc_fade_gain_ctr fade_ctr;
};

static void audio_anc_real_time_adaptive_data_packet(struct icsd_rtanc_tool_data *rtanc_tool);

static struct audio_rt_anc_hdl	*hdl = NULL;

anc_packet_data_t *rtanc_tool_data = NULL;

#if AUDIO_RT_ANC_PARAM_BY_TOOL_DEBUG
//修改默认值
float rtanc_debug_param[10] = {
    -2.0f,  // MSE update thr
    200.0f,  // 50Hz jitter thr
    2.0f,  // dither thr
    2.0f,  // target update thr
    7.0f,  // pz self flt dB thr < -10

    7.0f,  // pz self flt dB thr > -10
    -2.0f,  // pz self flt num thr
    1.0f,  // speak flag
    0.0f,  // vod print
    1.0f,
};

void audio_rtanc_debug_param_set(u8 cmd, float param)
{
    u8 max_num = ARRAY_SIZE(rtanc_debug_param);
    if (cmd > max_num) {
        rtanc_log("Err : rtanc debug param set fail, size limit %d > %d\n", \
                  cmd, max_num);
        return;
    }
    rtanc_debug_param[cmd] = param;
    rtanc_log("cmd %d, %d/100\n", cmd, (int)(rtanc_debug_param[cmd] * 100.0));
}
#endif

void audio_anc_real_time_adaptive_init(audio_anc_t *param)
{
    hdl = zalloc(sizeof(struct audio_rt_anc_hdl));
    hdl->param = param;
    hdl->fade_ctr.lff_gain = 16384;
    hdl->fade_ctr.lfb_gain = 16384;
    hdl->fade_ctr.rff_gain = 16384;
    hdl->fade_ctr.rfb_gain = 16384;

    hdl->rtanc_mode = ADT_REAL_TIME_ADAPTIVE_ANC_MODE;	//默认为正常模式

#if AUDIO_RT_ANC_PARAM_BY_TOOL_DEBUG
    hdl->debug_param = rtanc_debug_param;
#endif
}

/* Real Time ANC 自适应启动限制 */
int audio_rtanc_permit(void)
{
    if (anc_ext_ear_adaptive_param_check()) { //没有自适应参数
        return 1;
    }
    if (hdl->param->mode != ANC_ON) {	//非ANC模式
        return 2;
    }
    if (audio_anc_real_time_adaptive_state_get()) { //重入保护
        return 3;
    }
    if (anc_mode_switch_lock_get()) { //其他切模式过程不支持
        return 4;
    }
#if TCFG_AUDIO_FREQUENCY_GET_ENABLE
    if (audio_icsd_afq_is_running()) {	//AFQ 运行过程中不支持
        return 5;
    }
#endif
    if (hdl->param->lff_yorder > RT_ANC_MAX_ORDER || \
        hdl->param->lfb_yorder > RT_ANC_MAX_ORDER || \
        hdl->param->lcmp_yorder > RT_ANC_MAX_ORDER || \
        hdl->param->rff_yorder > RT_ANC_MAX_ORDER || \
        hdl->param->rfb_yorder > RT_ANC_MAX_ORDER || \
        hdl->param->rcmp_yorder > RT_ANC_MAX_ORDER) {
        return 6;						//最大滤波器个数限制
    }
    return 0;
}

//将滤波器等相关信息，整合成库需要的格式
void rt_anc_get_param(__rt_anc_param *rt_param_l, __rt_anc_param *rt_param_r)
{
    g_printf("%s, %d\n", __func__, __LINE__);

    int yorder_size = 5 * sizeof(double);
    audio_anc_t *param = hdl->param;

    if (rt_param_l) {
        rt_param_l->ff_yorder = param->lff_yorder;
        rt_param_l->fb_yorder = param->lfb_yorder;
        rt_param_l->cmp_yorder = param->lcmp_yorder;
        rt_param_l->ffgain = param->gains.l_ffgain;
        rt_param_l->fbgain = param->gains.l_fbgain;
        rt_param_l->cmpgain = param->gains.l_cmpgain;
        rt_param_l->ff_fade_gain = ((float)hdl->fade_ctr.lff_gain) / 16384.0f;
        rt_param_l->fb_fade_gain = ((float)hdl->fade_ctr.lfb_gain) / 16384.0f;

        memcpy(rt_param_l->ff_coeff, param->lff_coeff, rt_param_l->ff_yorder * yorder_size);
        memcpy(rt_param_l->fb_coeff, param->lfb_coeff, rt_param_l->fb_yorder * yorder_size);
        memcpy(rt_param_l->cmp_coeff, param->lcmp_coeff, rt_param_l->cmp_yorder * yorder_size);
        rt_param_l->param = param;

        rtanc_log("rtanc l order:ff %d, fb %d, cmp %d\n", rt_param_l->ff_yorder, \
                  rt_param_l->fb_yorder, rt_param_l->cmp_yorder);
        rtanc_log("rtanc l gain:ff %d/100, fb %d/100, cmp %d/100\n", (int)(rt_param_l->ffgain * 100.0f), \
                  (int)(rt_param_l->fbgain * 100.0f), (int)(rt_param_l->cmpgain * 100.0f));
        rtanc_log("rtanc l fade_gain:ff %d/100, fb %d/100\n", (int)(rt_param_l->ff_fade_gain * 100.0f), \
                  (int)(rt_param_l->fb_fade_gain * 100.0f));
        /* put_buf((u8*)rt_param_l->ff_coeff,  rt_param_l->ff_yorder * yorder_size); */
        /* put_buf((u8*)rt_param_l->fb_coeff,  rt_param_l->fb_yorder * yorder_size); */
        /* put_buf((u8*)rt_param_l->cmp_coeff, rt_param_l->cmp_yorder * yorder_size); */
    }

    if (rt_param_r) {
        rt_param_r->ff_yorder = param->lff_yorder;
        rt_param_r->fb_yorder = param->lfb_yorder;
        rt_param_r->cmp_yorder = param->lcmp_yorder;
        rt_param_r->ffgain = param->gains.l_ffgain;
        rt_param_r->fbgain = param->gains.l_fbgain;
        rt_param_r->cmpgain = param->gains.l_cmpgain;
        rt_param_r->ff_fade_gain = ((float)hdl->fade_ctr.rff_gain) / 16384.0f;
        rt_param_r->fb_fade_gain = ((float)hdl->fade_ctr.rfb_gain) / 16384.0f;

        memcpy(rt_param_r->ff_coeff, param->lff_coeff, rt_param_r->ff_yorder * yorder_size);
        memcpy(rt_param_r->fb_coeff, param->lfb_coeff, rt_param_r->fb_yorder * yorder_size);
        memcpy(rt_param_r->cmp_coeff, param->lcmp_coeff, rt_param_r->cmp_yorder * yorder_size);
        rt_param_r->param = param;
    }

}

//设置RTANC初始参数
int audio_adt_rtanc_set_infmt(void *rtanc_tool)
{
    rtanc_log("=================audio_adt_rtanc_set_infmt==================\n");
    if (++hdl->seq == 0xff) {
        hdl->seq = 0;
    }
    struct rt_anc_infmt infmt;
    hdl->rtanc_tool = rtanc_tool;
    infmt.id = hdl->seq;
    infmt.param = hdl->param;
    infmt.ext_cfg = (void *)anc_ext_ear_adaptive_cfg_get();
    infmt.anc_param_l = zalloc(sizeof(__rt_anc_param));
    //gali
    infmt.anc_param_r = zalloc(sizeof(__rt_anc_param));
    rt_anc_get_param(infmt.anc_param_l, infmt.anc_param_r);

    extern void rt_anc_set_init(struct rt_anc_infmt * fmt);
    rt_anc_set_init(&infmt);

#if AUDIO_RT_ANC_PARAM_BY_TOOL_DEBUG
    for (int i = 0; i < ARRAY_SIZE(rtanc_debug_param); i++) {
        rtanc_log("cmd %d, %d/100\n", i, (int)(hdl->debug_param[i] * 100.0));
        RTANC_SD_CFG->debug[i] = hdl->debug_param[i];
    }
#endif
    if (infmt.anc_param_l) {
        free(infmt.anc_param_l);
    }
    if (infmt.anc_param_r) {
        free(infmt.anc_param_r);
    }
    return 0;
}

static void audio_rt_anc_param_updata(void *rt_param_l, void *rt_param_r)
{
    rtanc_log("audio_rt_anc_param_updata\n");

    audio_anc_t *param = hdl->param;

#if ANC_EAR_ADAPTIVE_CMP_EN
    struct anc_cmp_param_output cmp_p;
    audio_rtanc_adaptive_cmp_output_get(&cmp_p);
    param->gains.l_cmpgain = cmp_p.l_gain;
    param->lcmp_coeff = cmp_p.l_coeff;
    rtanc_log("updata gain = %d, coef = %d, ", (int)(param->gains.l_cmpgain * 100), (int)(param->lcmp_coeff[0] * 100));
#endif

    __rt_anc_param *anc_param = (__rt_anc_param *)rt_param_l;
    param->gains.l_ffgain = anc_param->ffgain;
    param->lff_coeff = anc_param->ff_coeff;
    param->gains.l_fbgain = anc_param->fbgain;

    //param->lfb_coeff = &anc_param->lfb_coeff[0];
#if ANC_CONFIG_RFB_EN
    if (rt_param_r) {
        anc_param = (__rt_anc_param *)rt_param_r;
        param->gains.r_ffgain = anc_param->ffgain;
        param->rff_coeff = anc_param->ff_coeff;
        param->gains.r_fbgain = anc_param->fbgain;

#if ANC_EAR_ADAPTIVE_CMP_EN
        param->gains.r_cmpgain = cmp_p.r_gain;
        param->rcmp_coeff = cmp_p.r_coeff;
#endif
    }
#endif

    /* audio_anc_coeff_smooth_update(); */
    anc_coeff_online_update(hdl->param, 0);			//更新ANC滤波器
}

static void audio_rt_anc_iir_type_get(void)
{
    struct anc_ext_ear_adaptive_param *ext_cfg = anc_ext_ear_adaptive_cfg_get();
    if (ext_cfg) {
        for (int i = 0; i < 10; i++) {
            if (ext_cfg->ff_iir_high) {
                hdl->lff_iir_type[i] = ext_cfg->ff_iir_high[i].type;
            }
            if (ext_cfg->rff_iir_high) {
                hdl->rff_iir_type[i] = ext_cfg->rff_iir_high[i].type;
            }
            /* rtanc_log("get lff_type %d, rff_type %d\n", hdl->lff_iir_type[i], hdl->rff_iir_type[i]); */
        }
    }
}

//实时自适应算法输出接口
void audio_adt_rtanc_output_handle(void *rt_param_l, void *rt_param_r)
{
    //挂起RT_ANC
    rtanc_log("RTANC_STATE:OUTPUT_RUN\n");
    hdl->run_busy = 1;
    audio_anc_real_time_adaptive_suspend();

    //整理SZ数据结构
    __afq_output output = {0};// = zalloc(sizeof(__afq_output));// = &temp;
    output.sz_l = zalloc(sizeof(__afq_data));
    output.sz_l->len = 120;
    output.sz_l->out = zalloc(sizeof(float) * 120);
    icsd_rtanc_alg_get_sz(output.sz_l->out, 0); //0:L  1:R

    //put_buf((u8 *)output.sz_l->out, 120 * 4);

    //根据SZ计算CMP，并更新FF/FB/CMP等滤波器
#if ANC_EAR_ADAPTIVE_CMP_EN
    //CMP计算成功更新滤波器，否则可能处于强退状态，则不更新
    if (audio_anc_ear_adaptive_cmp_run(&output, CMP_FROM_RTANC) == 0) {
        audio_rt_anc_param_updata(rt_param_l, rt_param_r);
    }
#else
    audio_rt_anc_param_updata(rt_param_l, rt_param_r);
#endif

#if TCFG_AUDIO_ADAPTIVE_EQ_ENABLE
    //如果AEQ在运行就挂起RTANC
    if (audio_adaptive_eq_is_running()) {
        audio_anc_real_time_adaptive_suspend();
    }
#endif
    //执行挂在到AFQ 的算法，如AEQ, 内部会挂起RTANC
    audio_afq_common_output_post_msg(&output);

    free(output.sz_l->out);
    free(output.sz_l);
#if AUDIO_RT_ANC_EXPORT_TOOL_DATA_DEBUG
    audio_anc_real_time_adaptive_data_packet(hdl->rtanc_tool);
#endif

    //恢复RT_ANC
    audio_anc_real_time_adaptive_resume();

    rtanc_log("RTANC_STATE:OUTPUT_RUN END\n");
    hdl->run_busy = 0;
}

//RT ANC挂起接口
void audio_anc_real_time_adaptive_suspend(void)
{
    if (audio_anc_real_time_adaptive_state_get()) {
        hdl->suspend_cnt++;
        if ((hdl->suspend_cnt == 1) && (hdl->state == RT_ANC_STATE_OPEN)) {
            rtanc_log("RTANC_STATE:SUSPEND\n");
            hdl->state = RT_ANC_STATE_SUSPEND;
            if (hdl->reset_busy) {
                icsd_adt_rtanc_suspend();
            }
            rtanc_log("%s succ\n", __func__);
        }
    }
}

//RT ANC恢复接口
void audio_anc_real_time_adaptive_resume(void)
{
    if (audio_anc_real_time_adaptive_state_get()) {
        if (hdl->suspend_cnt) {
            hdl->suspend_cnt--;
            if ((hdl->suspend_cnt == 0) && (hdl->state == RT_ANC_STATE_SUSPEND)) {
                rtanc_log("RTANC_STATE:SUSPEND->OPEN\n");
                hdl->state = RT_ANC_STATE_OPEN;
                if (hdl->reset_busy) {
                    icsd_adt_rtanc_resume();
                }
                rtanc_log("%s succ\n", __func__);
            }
        }
    }
}

int audio_anc_real_time_adaptive_suspend_get(void)
{
    if (hdl) {
        return hdl->suspend_cnt;
    }
    return 0;
}

int audio_rtanc_adaptive_en(u8 en)
{
    int ret;
    if (en) {
        rtanc_log("=================rt_anc_open==================\n");
        rtanc_log("rtanc state %d\n", audio_anc_real_time_adaptive_state_get());
        ret = audio_rtanc_permit();
        if (ret) {
            rtanc_log("Err:rt_anc_open permit %d\n", ret);
            return 1;
        }

        hdl->run_busy = 0;
        hdl->suspend_cnt = 0;
        rtanc_log("RTANC_STATE:OPEN\n");
        hdl->state = RT_ANC_STATE_OPEN;
        clock_alloc("ANC_RT_ADAPTIVE", 48 * 1000000L);
        audio_rt_anc_iir_type_get();	//获取工具配置IIR TYPE
#if ANC_EAR_ADAPTIVE_CMP_EN
        audio_anc_ear_adaptive_cmp_open();
#endif
        return audio_icsd_adt_sync_open(hdl->rtanc_mode);

    } else {
        if (audio_anc_real_time_adaptive_state_get() == 0) {
            return 0;
        }
        rtanc_log("================rt_anc_close==================\n");
        rtanc_log("RTANC_STATE:CLOSE\n");
        hdl->state = RT_ANC_STATE_CLOSE;
        audio_icsd_adt_sync_close(ADT_REAL_TIME_ADAPTIVE_ANC_MODE | ADT_REAL_TIME_ADAPTIVE_ANC_TIDY_MODE, 0);
        u8 wait_cnt = 100;
        while (hdl->run_busy && wait_cnt) {
            wait_cnt--;
            os_time_dly(1);
        }
        if (!wait_cnt) {
            rtanc_log("ERR!!rt_anc_close timeout\n");
        }
        clock_free("ANC_RT_ADAPTIVE");

#if ANC_EAR_ADAPTIVE_CMP_EN
        audio_anc_ear_adaptive_cmp_close();
#endif
        rtanc_log("rt_anc_close okn");

    }
    return 0;
}

//获取用户端是否启用这个功能
u8 audio_rtanc_app_func_en_get(void)
{
    return hdl->app_func_en;
}

int audio_anc_real_time_adaptive_open(void)
{
    hdl->app_func_en = 1;
    return audio_rtanc_adaptive_en(1);
}

int audio_anc_real_time_adaptive_close(void)
{
    int ret;
    hdl->app_func_en = 0;
    ret = audio_rtanc_adaptive_en(0);
    //恢复默认ANC参数
    if (anc_mode_get() == ANC_ON) {
#if ANC_MULT_ORDER_ENABLE
        /* audio_anc_mult_scene_set(audio_anc_mult_scene_get()); */
        audio_anc_mult_scene_update(audio_anc_mult_scene_get());
#else
        audio_anc_db_cfg_read();
        audio_anc_coeff_smooth_update();
#endif
    }
    return ret;
}

#if AUDIO_RT_ANC_EXPORT_TOOL_DATA_DEBUG
static void audio_anc_real_time_adaptive_data_packet(struct icsd_rtanc_tool_data *rtanc_tool)
{
    int len = rtanc_tool->h_len;
    int ff_yorder = hdl->param->lff_yorder;

    int ff_dat_len =  sizeof(anc_fr_t) * ff_yorder + 4;

#if ANC_EAR_ADAPTIVE_CMP_EN
    int cmp_yorder = ANC_ADAPTIVE_CMP_ORDER;
    int cmp_dat_len =  sizeof(anc_fr_t) * cmp_yorder + 4;
#endif

    u8 *ff_dat, *cmp_dat, *rff_dat, *rcmp_dat;

#if ANC_EAR_ADAPTIVE_CMP_EN
    float *lcmp_output = audio_anc_ear_adaptive_cmp_output_get(ANC_EAR_ADAPTIVE_CMP_CH_L);
    float *rcmp_output = audio_anc_ear_adaptive_cmp_output_get(ANC_EAR_ADAPTIVE_CMP_CH_R);
#endif

#if ANC_CONFIG_LFF_EN
    ff_dat = zalloc(ff_dat_len);
    //RTANC 增益没有输出符号，需要对齐工具符号
    rtanc_tool->ff_fgq_l[0] = (hdl->param->gains.gain_sign & ANCL_FF_SIGN) ? \
                              (0 - rtanc_tool->ff_fgq_l[0]) : rtanc_tool->ff_fgq_l[0];
    audio_anc_fr_format(ff_dat, rtanc_tool->ff_fgq_l, ff_yorder, hdl->lff_iir_type);
#endif/*ANC_CONFIG_LFF_EN*/
#if ANC_CONFIG_LFB_EN && ANC_EAR_ADAPTIVE_CMP_EN
    cmp_dat = zalloc(cmp_dat_len);
    audio_anc_fr_format(cmp_dat, lcmp_output, cmp_yorder, icsd_cmp_type);
#endif/*ANC_EAR_ADAPTIVE_CMP_EN*/
#if ANC_CONFIG_RFF_EN
    rff_dat = zalloc(ff_dat_len);
    //RTANC 增益没有输出符号，需要对齐工具符号
    rtanc_tool->ff_fgq_r[0] = (hdl->param->gains.gain_sign & ANCR_FF_SIGN) ? \
                              (0 - rtanc_tool->ff_fgq_r[0]) : rtanc_tool->ff_fgq_r[0];
    audio_anc_fr_format(rff_dat, rt_anc_tool->ff_fgq_r, ff_yorder, hdl->rff_iir_type);
#endif/*ANC_CONFIG_RFF_EN*/
#if ANC_CONFIG_RFB_EN && ANC_EAR_ADAPTIVE_CMP_EN
    rcmp_dat = zalloc(cmp_dat_len);
    audio_anc_fr_format(rcmp_dat, rcmp_output, cmp_yorder, icsd_cmp_type);
#endif/*ANC_EAR_ADAPTIVE_CMP_EN*/

    if (hdl->param->developer_mode || anc_ext_tool_online_get()) {
        rtanc_log("-- len = %d\n", len);
        rtanc_log("-- ff_yorder = %d\n", ff_yorder);
#if ANC_EAR_ADAPTIVE_CMP_EN
        rtanc_log("-- cmp_yorder = %d\n", cmp_yorder);
#endif
        /* 先统一申请空间，因为下面不知道什么情况下调用函数 anc_data_catch 时令参数 init_flag 为1 */
        rtanc_tool_data = anc_data_catch(rtanc_tool_data, NULL, 0, 0, 1);

#if TCFG_USER_TWS_ENABLE
        if (bt_tws_get_local_channel() == 'R') {
            r_printf("RTANC export send tool_data, ch:R\n");
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->h_freq, len * 4, ANC_R_ADAP_FRE, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->sz_out_l, len * 8, ANC_R_ADAP_SZPZ, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->pz_out_l, len * 8, ANC_R_ADAP_PZ, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->target_out_l, len * 8, ANC_R_ADAP_TARGET, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->target_out_cmp_l, len * 8, ANC_R_ADAP_TARGET_CMP, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)ff_dat, ff_dat_len, ANC_R_FF_IIR, 0);  //R_ff
#if ANC_CONFIG_LFB_EN && ANC_EAR_ADAPTIVE_CMP_EN
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)cmp_dat, cmp_dat_len, ANC_R_CMP_IIR, 0);  //R_cmp
#endif/*ANC_EAR_ADAPTIVE_CMP_EN*/
        } else
#endif/*TCFG_USER_TWS_ENABLE*/
        {
            r_printf("RTANC export send tool_data, ch:L\n");

            /* put_buf((u8 *)rtanc_tool->h_freq, len * 4); */
            /* put_buf((u8 *)rtanc_tool->sz_out_l, len * 8); */
            /* put_buf((u8 *)rtanc_tool->pz_out_l, len * 8); */
            /* put_buf((u8 *)rtanc_tool->target_out_l, len * 8); */
            /* put_buf((u8 *)rtanc_tool->target_out_cmp_l, len * 8); */
            /* put_buf((u8 *)ff_dat, ff_dat_len);  //R_ff */
            /* put_buf((u8 *)cmp_dat, cmp_dat_len);  //R_cmp */

            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->h_freq, len * 4, ANC_L_ADAP_FRE, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->sz_out_l, len * 8, ANC_L_ADAP_SZPZ, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->pz_out_l, len * 8, ANC_L_ADAP_PZ, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->target_out_l, len * 8, ANC_L_ADAP_TARGET, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->target_out_cmp_l, len * 8, ANC_L_ADAP_TARGET_CMP, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)ff_dat, ff_dat_len, ANC_L_FF_IIR, 0);  //R_ff
#if ANC_CONFIG_LFB_EN && ANC_EAR_ADAPTIVE_CMP_EN
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)cmp_dat, cmp_dat_len, ANC_L_CMP_IIR, 0);  //R_cmp
#endif/*ANC_EAR_ADAPTIVE_CMP_EN*/
        }
    }

#if ANC_CONFIG_LFF_EN
    free(ff_dat);
#endif/*ANC_CONFIG_LFF_EN*/
#if ANC_CONFIG_LFB_EN && ANC_EAR_ADAPTIVE_CMP_EN
    free(cmp_dat);
#endif/*ANC_EAR_ADAPTIVE_CMP_EN*/
#if ANC_CONFIG_RFF_EN
    free(rff_dat);
#endif/*ANC_CONFIG_RFF_EN*/
#if ANC_CONFIG_RFB_EN && ANC_EAR_ADAPTIVE_CMP_EN
    free(rcmp_dat);
#endif/*ANC_EAR_ADAPTIVE_CMP_EN*/

}
#endif

int audio_anc_real_time_adaptive_tool_data_get(u8 **buf, u32 *len)
{
    if (rtanc_tool_data == NULL) {
        rtanc_log("rtanc packet is NULL, return!\n");
        return -1;
    }
    if (rtanc_tool_data->dat_len == 0) {
        rtanc_log("rtanc error: dat_len == 0\n");
        return -1;
    }
    g_printf("rtanc_tool_data->dat_len %d\n", rtanc_tool_data->dat_len);
    *buf = rtanc_tool_data->dat;
    *len = rtanc_tool_data->dat_len;
    return 0;
}

int audio_rtanc_fade_gain_suspend(struct rt_anc_fade_gain_ctr *fade_ctr)
{
    g_printf("%s, state %d\n", __func__, audio_anc_real_time_adaptive_state_get());
    if ((!audio_anc_real_time_adaptive_state_get()) || hdl->reset_busy) {
        hdl->fade_ctr = *fade_ctr;
        return 0;
    }
    if (hdl) {
        if (memcmp(&hdl->fade_ctr, fade_ctr, sizeof(struct rt_anc_fade_gain_ctr))) {
            hdl->fade_gain_suspend = 1;
            audio_anc_real_time_adaptive_suspend();

            struct rtanc_param *rtanc_p;

            rtanc_p = zalloc(sizeof(struct rtanc_param));

            rtanc_p->ffl_filter.fade_gain = ((float)fade_ctr->lff_gain) / 16384.0f;
            rtanc_p->fbl_filter.fade_gain = ((float)fade_ctr->lfb_gain) / 16384.0f;
            rtanc_p->ffr_filter.fade_gain = ((float)fade_ctr->rff_gain) / 16384.0f;
            rtanc_p->fbr_filter.fade_gain = ((float)fade_ctr->rfb_gain) / 16384.0f;

            icsd_alg_rtanc_fadegain_update(rtanc_p);
            hdl->fade_ctr = *fade_ctr;
            free(rtanc_p);
        }
    }
    return 0;
}

int audio_rtanc_fade_gain_resume(void)
{
    if (hdl->fade_gain_suspend) {
        hdl->fade_gain_suspend = 0;
        audio_anc_real_time_adaptive_resume();
    }
    return 0;
}

static u8 RTANC_idle_query(void)
{
    if (hdl) {
        return (hdl->state) ? 0 : 1;
    }
    return 1;
}

u8 audio_anc_real_time_adaptive_state_get(void)
{
    return (!RTANC_idle_query());
}

u8 audio_anc_real_time_adaptive_run_busy_get(void)
{
    if (hdl) {
        return hdl->run_busy;
    }
    return 0;
}

REGISTER_LP_TARGET(RTANC_lp_target) = {
    .name       = "RTANC",
    .is_idle    = RTANC_idle_query,
};

//复位RTANC模式，阻塞处理
int audio_anc_real_time_adaptive_reset(int rtanc_mode)
{
    int wait_cnt, switch_busy, mode_cmp, close_mode;
    if (hdl->rtanc_mode == rtanc_mode) {
        rtanc_log("Err:rtanc adaptive reset fail, same mode\n");
        return -1;
    }
    hdl->rtanc_mode = rtanc_mode;
    if (audio_anc_real_time_adaptive_state_get()) {
        u32 last = jiffies_usec();
        rtanc_log("%s, mode = %x start\n", __func__, rtanc_mode);
        //挂起RTANC
        audio_anc_real_time_adaptive_suspend();
        hdl->reset_busy = 1;
        //设置为默认参数, 但是不更新
        /* audio_anc_mult_scene_update(audio_anc_mult_scene_get()); */
        audio_anc_mult_scene_set(audio_anc_mult_scene_get());	//耗时30ms
        //修改ADT MODE 目标模式
        icsd_adt_rt_anc_mode_set(rtanc_mode);
        //复位ADT
        if (rtanc_mode == ADT_REAL_TIME_ADAPTIVE_ANC_TIDY_MODE) {
            audio_icsd_adt_sync_close(ADT_REAL_TIME_ADAPTIVE_ANC_MODE, 0);
            close_mode = ADT_REAL_TIME_ADAPTIVE_ANC_MODE;
        } else {
            audio_icsd_adt_sync_close(ADT_REAL_TIME_ADAPTIVE_ANC_TIDY_MODE, 0);
            close_mode = ADT_REAL_TIME_ADAPTIVE_ANC_TIDY_MODE;
        }
        //等待切换完毕
        wait_cnt = 200;
        switch_busy = 1;
        while (switch_busy && wait_cnt) {
            mode_cmp = get_icsd_adt_mode() & close_mode;
            switch_busy = get_icsd_adt_mode_switch_busy() | mode_cmp;
            //rtanc_log("adt_mode 0x%x, close_mode %x, s_busy %d\n", get_icsd_adt_mode(), close_mode, get_icsd_adt_mode_switch_busy());
            wait_cnt--;
            os_time_dly(1);
        }
        if (!wait_cnt) {
            rtanc_log("ERR: %s timeout err!!\n", __func__);
        }
        //恢复RTANC
        hdl->reset_busy = 0;
        audio_anc_real_time_adaptive_resume();
        rtanc_log("%s, end, %d\n", __func__, jiffies_usec2offset(last, jiffies_usec()));
    }
    return 0;
}

//测试demo
int audio_anc_real_time_adaptive_demo(void)
{
#if 1//tone
    if (hdl->state == RT_ANC_STATE_CLOSE) {
        if (audio_anc_real_time_adaptive_open()) {
            icsd_adt_tone_play(ICSD_ADT_TONE_NUM0);
        } else {
            icsd_adt_tone_play(ICSD_ADT_TONE_NUM1);
        }
    } else {
        icsd_adt_tone_play(ICSD_ADT_TONE_NUM0);
        audio_anc_real_time_adaptive_close();
    }
#else
    if (hdl->state == RT_ANC_STATE_CLOSE) {
        audio_anc_real_time_adaptive_open();
    } else {
        audio_anc_real_time_adaptive_close();
    }
#endif
    return 0;
}

#endif
