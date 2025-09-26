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
#include "audio_anc_debug_tool.h"
#include "mic_effect.h"

#include "icsd_adt.h"
#include "icsd_adt_app.h"
#include "audio_anc_common_plug.h"

#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif/*TCFG_USER_TWS_ENABLE*/

#if TCFG_AUDIO_FREQUENCY_GET_ENABLE
#include "icsd_afq_app.h"
#endif

#if TCFG_AUDIO_ANC_ADAPTIVE_CMP_EN
#include "icsd_cmp_app.h"
#endif

#if TCFG_AUDIO_ADAPTIVE_EQ_ENABLE
#include "icsd_aeq_app.h"
#endif

#if TCFG_SPEAKER_EQ_NODE_ENABLE
#include "effects/audio_spk_eq.h"
#endif

#if AUDIO_ANC_DATA_EXPORT_VIA_UART
#include "audio_anc_develop.h"
#endif

#if 1
#define rtanc_log	printf
#else
#define rtanc_log(...)
#endif/*log_en*/

#if 0
#define rtanc_debug_log	printf
#else
#define rtanc_debug_log(...)
#endif/*log_en*/


#define AUDIO_RT_ANC_SUSPEND_OBJECT_PRINTF			1		//RTANC 挂起对象打印

enum {
    RTANC_SZ_STA_INIT = BIT(0),		//SZ_SEL 初始化
    RTANC_SZ_STA_AFQ_OUT = BIT(1),	//SZ_SEL AFQ 输出SZ
    RTANC_SZ_STA_START = BIT(2),	//SZ_SEL RTANC 启动
};

enum {
    RTANC_SPP_CMD_SELF_STATE = 0,
    RTANC_SPP_CMD_SUSPEND_STATE,
    RTANC_SPP_CMD_RESUME_STATE,
};

//SE_SEL
const u8 sz_sel_h_freq_test[100] = {
    0x00, 0x1B, 0x37, 0x42, 0x00, 0x1B, 0xB7, 0x42, 0x40, 0x54, 0x09, 0x43, 0x00, 0x1B, 0x37, 0x43,
    0xC0, 0xE1, 0x64, 0x43, 0x40, 0x54, 0x89, 0x43, 0xA0, 0x37, 0xA0, 0x43, 0x00, 0x1B, 0xB7, 0x43,
    0x60, 0xFE, 0xCD, 0x43, 0xC0, 0xE1, 0xE4, 0x43, 0x20, 0xC5, 0xFB, 0x43, 0xF0, 0xC5, 0x14, 0x44,
    0x50, 0xA9, 0x2B, 0x44, 0xB0, 0x8C, 0x42, 0x44, 0x10, 0x70, 0x59, 0x44, 0x70, 0x53, 0x70, 0x44,
    0x68, 0x9B, 0x83, 0x44, 0x18, 0x0D, 0x8F, 0x44, 0xC8, 0x7E, 0x9A, 0x44, 0x28, 0x62, 0xB1, 0x44,
    0x88, 0x45, 0xC8, 0x44, 0xE8, 0x28, 0xDF, 0x44, 0x48, 0x0C, 0xF6, 0x44, 0x84, 0xE9, 0x11, 0x45,
    0xE4, 0xCC, 0x28, 0x45
};

#define AUDIO_RTANC_COEFF_SIZE				40 		//单个滤波器大小 5 * sizeof(double)

//SZ_SEL 触发计算
#define RTANC_SZ_STATE_TRIGGER		(RTANC_SZ_STA_INIT | RTANC_SZ_STA_AFQ_OUT | RTANC_SZ_STA_START)

//管理RTANC 滤波器输出:启动后RAM空间常驻
struct audio_rt_anc_output {
    u8 ff_use_outfgq;
    float lff_gain;
    float lfb_gain;
    float lcmp_gain;
    float lff_fgq[31];
    double *lff_coeff;
    double *lcmp_coeff;

#if ANC_CONFIG_RFB_EN
    float rff_gain;
    float rfb_gain;
    float rcmp_gain;
    double *rff_coeff;
    double *rcmp_coeff;
#endif
};

//管理SZ_SEL 相关数据结构
struct audio_rt_anc_sz_sel {
    u8 state;
    u8 sel_idx;
    float *lff_out;			//[31] gain + 10 * order_size
    float *lcmp_out;	    //[31] gain + 10 * order_size
    __afq_output output;	//sz_output
};

struct audio_rt_anc_hdl {
    u8 seq;
    u8 state;
    u8 run_busy;
    u8 app_func_en;
    u8 lff_iir_type[10];
    u8 rff_iir_type[10];
    u8 dut_mode;            //产测模式使能
    int rtanc_mode;
    audio_anc_t *param;
    float *debug_param;
    float *pz_cmp;
    float *sz_cmp;
#if TCFG_AUDIO_ANC_ADAPTIVE_CMP_EN
    u8 *cmp_tool_data;							//CMP工具数据缓存
    int cmp_tool_data_len;
#endif
    struct rtanc_param *rtanc_p;
    struct icsd_rtanc_tool_data *rtanc_tool;	//工具临时数据
    struct rt_anc_fade_gain_ctr fade_ctr;
    struct audio_rt_anc_output out;				//RTANC/CMP IIR 滤波器输出
#if TCFG_SPEAKER_EQ_NODE_ENABLE
    struct eq_default_seg_tab spk_eq_tab_l;
    struct eq_default_seg_tab spk_eq_tab_r;
#endif
#if ANC_DUT_MIC_CMP_GAIN_ENABLE
    struct anc_mic_gain_cmp_cfg *mic_cmp;
#endif
    struct audio_rt_anc_sz_sel sz_sel;
    __rtanc_var_buffer var_cache_buff;         //缓存上传算法结果，便于重新传入

    struct list_head suspend_head;
    OS_MUTEX mutex;
};

struct rtanc_suspend_bulk {
    const char *name;
    struct list_head entry;
};

static void audio_anc_real_time_adaptive_data_packet(struct icsd_rtanc_tool_data *rtanc_tool);
static void audio_rtanc_sz_pz_cmp_process(void);
static void audio_rtanc_suspend_list_query(u8 init_flag);
static void audio_rtanc_spp_send_data(u8 cmd, u8 *buf, int len);
static void audio_rtanc_cmp_tool_data_catch(u8 *data, int len);

static struct audio_rt_anc_hdl	*hdl = NULL;

anc_packet_data_t *rtanc_tool_data = NULL;

struct rtanc_in_ancoff_handle {
    u8 state;//记录打开的状态
    u8 busy;
    u32 open_timer_id; //记录定时器id
    u32 close_timer_id; //记录定时器id
    u8 ancoff_open_rtanc_state;//记录是否允许ancoff下打开rtanc
    u32 open_times; //配置从关闭到打开间隔时间，即关闭rtanc的时间长度，ms
    u32 close_times; //配置从打开到关闭间隔时间，即打开rtanc的时间长度，ms
    u8 close_output_cnt;//配置目标输出关闭次数
    u8 output_cnt;//记录输出关闭次数
};
struct rtanc_in_ancoff_handle *rtanc_in_ancoff_hdl = NULL;

#if 1
#define ancoff_rtanc_log	r_printf
#else
#define ancoff_rtanc_log(...)
#endif/*log_en*/

#define ANCOFF_OPEN_RTANC_TIMES     (1000 * 10) //anc off 开 rtanc的时间长度，ms
#define ANCOFF_CLOSE_RTANC_TIMES    (1000 * 5 * 60)//anc off 关 rtanc的时间长度，ms
#define ANCOFF_CLOSE_RTANC_OUTPUT_CNT  0//(1) //anc off 关rtanc的输出次数

static void ancoff_close_rtanc_timer(void *p);
static void ancoff_open_rtanc_timer(void *p)
{
    struct rtanc_in_ancoff_handle *hdl_t = rtanc_in_ancoff_hdl;
    if (anc_mode_get() != ANC_OFF) {
        return;
    }
    ancoff_rtanc_log("ancoff_open_rtanc_timer\n");
    if (hdl_t && hdl_t->state) {
        hdl_t->busy = 1;
        hdl_t->open_timer_id = 0;

        hdl_t->ancoff_open_rtanc_state = 1;//配置当前允许anc off 开rtanc
        //anc off时，开关rtanc
        set_rtanc_reset_flag(1);//处理切相同模式
        anc_mode_switch(ANC_OFF, 0);
        set_rtanc_reset_flag(0);

#if ANCOFF_CLOSE_RTANC_TIMES
        //重新定时关闭rtanc
        if (hdl_t->close_timer_id) {
            sys_timeout_del(hdl_t->close_timer_id);
            hdl_t->close_timer_id = 0;
        }
        hdl_t->close_timer_id = sys_timeout_add((void *)hdl_t, ancoff_close_rtanc_timer, hdl_t->close_times);
#endif

        hdl_t->busy = 0;
    }
}

static void ancoff_close_rtanc_timer(void *p)
{
    struct rtanc_in_ancoff_handle *hdl_t = rtanc_in_ancoff_hdl;
    if (anc_mode_get() != ANC_OFF) {
        return;
    }
    ancoff_rtanc_log("ancoff_close_rtanc_timer\n");
    if (hdl_t && hdl_t->state) {
        hdl_t->busy = 1;
        hdl_t->close_timer_id = 0;

        //anc off 关闭rtanc
        hdl_t->ancoff_open_rtanc_state = 0;//配置当前需要anc off 关闭rtanc
        //anc off时，开关rtanc
        audio_rtanc_anc_off_switch();

        //重新定时打开rtanc
        if (hdl_t->open_timer_id) {
            sys_timeout_del(hdl_t->open_timer_id);
            hdl_t->open_timer_id = 0;
        }
        hdl_t->open_timer_id = sys_timeout_add((void *)hdl_t, ancoff_open_rtanc_timer, hdl_t->open_times);

        hdl_t->busy = 0;
    }
}

int audio_rtanc_in_ancoff_open()
{
    ancoff_rtanc_log("audio_rtanc_in_ancoff_open \n");
    if (rtanc_in_ancoff_hdl) {
        ancoff_rtanc_log("rtanc_in_ancoff_hdl is already open!!!\n");
        return -1;
    }
    struct rtanc_in_ancoff_handle *hdl_t = anc_malloc("ICSD_RTANC", sizeof(struct rtanc_in_ancoff_handle));
    ASSERT(hdl_t);

    hdl_t->open_times = ANCOFF_CLOSE_RTANC_TIMES; //配置从关闭到打开间隔时间，即关闭rtanc的时间长度，ms
    hdl_t->close_times = ANCOFF_OPEN_RTANC_TIMES; //配置从打开到关闭间隔时间，即打开rtanc的时间长度，ms
    hdl_t->close_output_cnt = ANCOFF_CLOSE_RTANC_OUTPUT_CNT; //
    ancoff_rtanc_log("open times : %d ms, close times : %d ms, close output cnt : %d\n", hdl_t->open_times, hdl_t->close_times, hdl_t->close_output_cnt);

    hdl_t->open_timer_id = 0;//先不开open定时器，在输出计算里面开定时器
    hdl_t->output_cnt = 0;

#if ANCOFF_CLOSE_RTANC_TIMES
    hdl_t->close_timer_id = sys_timeout_add((void *)hdl_t, ancoff_close_rtanc_timer, hdl_t->close_times);
#endif

    hdl_t->ancoff_open_rtanc_state = 1;//配置当前允许anc off 开rtanc
    hdl_t->busy = 0;
    hdl_t->state = 1;
    rtanc_in_ancoff_hdl = hdl_t;
    return 0;

}
u8 get_ancoff_open_rtanc_state()
{
#if (ANCOFF_CLOSE_RTANC_OUTPUT_CNT || ANCOFF_CLOSE_RTANC_TIMES)
    struct rtanc_in_ancoff_handle *hdl_t = rtanc_in_ancoff_hdl;
    if (hdl_t && hdl_t->state) {
        return hdl_t->ancoff_open_rtanc_state;
    }
    return 0;
#else
    return 1;
#endif
}

int audio_rtanc_in_ancoff_close()
{
    struct rtanc_in_ancoff_handle *hdl_t = rtanc_in_ancoff_hdl;
    if (hdl_t) {
        hdl_t->state = 0;
        ancoff_rtanc_log("audio_rtanc_in_ancoff_close\n");
        while (hdl_t->busy) {
            putchar('w');
            os_time_dly(1);
        }
        if (hdl_t->open_timer_id) {
            sys_timeout_del(hdl_t->open_timer_id);
            hdl_t->open_timer_id = 0;
        }
#if ANCOFF_CLOSE_RTANC_TIMES
        if (hdl_t->close_timer_id) {
            sys_timeout_del(hdl_t->close_timer_id);
            hdl_t->close_timer_id = 0;
        }
#endif
        anc_free(hdl_t);
        rtanc_in_ancoff_hdl = NULL;
    }
    return 0;
}

static int audio_rtanc_in_ancoff_add_output_cnt(int add)
{
#if ANCOFF_CLOSE_RTANC_OUTPUT_CNT
    struct rtanc_in_ancoff_handle *hdl_t = rtanc_in_ancoff_hdl;
    if (anc_mode_get() != ANC_OFF) {
        return 0;
    }
    if (hdl_t && hdl_t->state) {
        hdl_t->busy = 1;
        hdl_t->output_cnt += add;
        ancoff_rtanc_log("audio_rtanc_in_ancoff_add_output_cnt: %d \n", hdl_t->output_cnt);
        if (hdl_t->output_cnt >= hdl_t->close_output_cnt) {
            hdl_t->output_cnt = 0;
#if ANCOFF_CLOSE_RTANC_TIMES
            //删除关闭定时器
            if (hdl_t->close_timer_id) {
                sys_timeout_del(hdl_t->close_timer_id);
                hdl_t->close_timer_id = 0;
            }
#endif

            //达到输出计数，anc off关闭rtanc
            hdl_t->ancoff_open_rtanc_state = 0;//配置当前不允许anc off 开rtanc
            set_rtanc_reset_flag(1);//处理切相同模式
            int err = os_taskq_post_msg(SPEAK_TO_CHAT_TASK_NAME, 3, ICSD_ANC_MODE_SWITCH, ANC_OFF, 0);
            if (err != OS_NO_ERR) {
                ancoff_rtanc_log("%s err %d", __func__, err);
            }

            //设置timeout 重新在anc off打开rtanc
            if (hdl_t->open_timer_id) {
                sys_timeout_del(hdl_t->open_timer_id);
                hdl_t->open_timer_id = 0;
            }
            hdl_t->open_timer_id = sys_timeout_add((void *)hdl_t, ancoff_open_rtanc_timer, hdl_t->open_times);
        }

        hdl_t->busy = 0;
    }
#endif
    return 0;
}


void audio_anc_real_time_adaptive_init(audio_anc_t *param)
{
    hdl = anc_malloc("ICSD_RTANC", sizeof(struct audio_rt_anc_hdl));
    hdl->param = param;
    hdl->fade_ctr.lff_gain = 16384;
    hdl->fade_ctr.lfb_gain = 16384;
    hdl->fade_ctr.rff_gain = 16384;
    hdl->fade_ctr.rfb_gain = 16384;

    hdl->rtanc_mode = ADT_REAL_TIME_ADAPTIVE_ANC_MODE;	//默认为正常模式

    os_mutex_create(&hdl->mutex);
    INIT_LIST_HEAD(&hdl->suspend_head);
}

/* Real Time ANC 自适应启动限制 */
int audio_rtanc_permit(u8 sync_mode)
{
    int ret = anc_ext_rtanc_param_check();
    if (ret) { //RTANC 工具参数缺失
        return ret;
    }
    if (hdl->param->lff_yorder > RT_ANC_MAX_ORDER || \
        hdl->param->lfb_yorder > RT_ANC_MAX_ORDER || \
        hdl->param->lcmp_yorder > RT_ANC_MAX_ORDER || \
        hdl->param->rff_yorder > RT_ANC_MAX_ORDER || \
        hdl->param->rfb_yorder > RT_ANC_MAX_ORDER || \
        hdl->param->rcmp_yorder > RT_ANC_MAX_ORDER) {
        return ANC_EXT_OPEN_FAIL_MAX_IIR_LIMIT;		//最大滤波器个数限制
    }
    return 0;
}

//将滤波器等相关信息，整合成库需要的格式
static void rt_anc_get_param(__rt_anc_param *rt_param_l, __rt_anc_param *rt_param_r)
{
    rtanc_debug_log("%s, %d\n", __func__, __LINE__);

    int yorder_size = 5 * sizeof(double);
    audio_anc_t *param = hdl->param;
    //ANC 0, 通透 1(通透只在DAC出声的时候更新)
    int anc_mode = (anc_mode_get() == ANC_ON) ? 0 : 1;

    if (rt_param_l) {
        if (anc_mode_get() == ANC_TRANSPARENCY) {
            rt_param_l->ff_use_outfgq = 0;
            // memcpy(rt_param_l->ff_fgq, hdl->out.lff_fgq, 31*4);
            rt_param_l->ff_yorder = param->ltrans_yorder;
            rt_param_l->fb_yorder = param->ltrans_fb_yorder;
            rt_param_l->cmp_yorder = param->ltrans_cmp_yorder;
            rt_param_l->ffgain = param->gains.l_transgain;
            rt_param_l->fbgain = param->ltrans_fbgain;
            rt_param_l->cmpgain = param->ltrans_cmpgain;

            memcpy(rt_param_l->ff_coeff, param->ltrans_coeff, rt_param_l->ff_yorder * yorder_size);
            memcpy(rt_param_l->fb_coeff, param->ltrans_fb_coeff, rt_param_l->fb_yorder * yorder_size);
            memcpy(rt_param_l->cmp_coeff, param->ltrans_cmp_coeff, rt_param_l->cmp_yorder * yorder_size);
        } else {
            rt_param_l->ff_use_outfgq = hdl->out.ff_use_outfgq;
            memcpy(rt_param_l->ff_fgq, hdl->out.lff_fgq, 31 * 4);
            rt_param_l->ff_yorder = param->lff_yorder;
            rt_param_l->fb_yorder = param->lfb_yorder;
            rt_param_l->cmp_yorder = param->lcmp_yorder;
            rt_param_l->ffgain = param->gains.l_ffgain;
            rt_param_l->fbgain = param->gains.l_fbgain;
            rt_param_l->cmpgain = param->gains.l_cmpgain;

            memcpy(rt_param_l->ff_coeff, param->lff_coeff, rt_param_l->ff_yorder * yorder_size);
            memcpy(rt_param_l->fb_coeff, param->lfb_coeff, rt_param_l->fb_yorder * yorder_size);
            memcpy(rt_param_l->cmp_coeff, param->lcmp_coeff, rt_param_l->cmp_yorder * yorder_size);
        }
        rt_param_l->ff_fade_gain = ((float)hdl->fade_ctr.lff_gain) / 16384.0f;
        rt_param_l->fb_fade_gain = ((float)hdl->fade_ctr.lfb_gain) / 16384.0f;
        rt_param_l->anc_mode = anc_mode;
        rt_param_l->param = param;
        //printf(" ================== hdl->dut_mode %d\n",  hdl->dut_mode);
        rt_param_l->test_mode = hdl->dut_mode;

        rtanc_debug_log("rtanc l order:ff %d, fb %d, cmp %d\n", rt_param_l->ff_yorder, \
                        rt_param_l->fb_yorder, rt_param_l->cmp_yorder);
        rtanc_debug_log("rtanc l gain:ff %d/100, fb %d/100, cmp %d/100\n", (int)(rt_param_l->ffgain * 100.0f), \
                        (int)(rt_param_l->fbgain * 100.0f), (int)(rt_param_l->cmpgain * 100.0f));
        rtanc_debug_log("rtanc l fade_gain:ff %d/100, fb %d/100\n", (int)(rt_param_l->ff_fade_gain * 100.0f), \
                        (int)(rt_param_l->fb_fade_gain * 100.0f));
        /* put_buf((u8*)rt_param_l->ff_coeff,  rt_param_l->ff_yorder * yorder_size); */
        /* put_buf((u8*)rt_param_l->fb_coeff,  rt_param_l->fb_yorder * yorder_size); */
        /* put_buf((u8*)rt_param_l->cmp_coeff, rt_param_l->cmp_yorder * yorder_size); */
    }

#if ANC_CONFIG_RFB_EN
    if (rt_param_r) { //右声道未完善
        if (anc_mode_get() == ANC_TRANSPARENCY) {

            rt_param_r->ff_yorder = param->rtrans_yorder;
            rt_param_r->fb_yorder = param->rtrans_fb_yorder;
            rt_param_r->cmp_yorder = param->rtrans_cmp_yorder;
            rt_param_r->ffgain = param->gains.r_transgain;
            rt_param_r->fbgain = param->rtrans_fbgain;
            rt_param_r->cmpgain = param->rtrans_cmpgain;

            memcpy(rt_param_r->ff_coeff, param->rtrans_coeff, rt_param_r->ff_yorder * yorder_size);
            memcpy(rt_param_r->fb_coeff, param->rtrans_fb_coeff, rt_param_r->fb_yorder * yorder_size);
            memcpy(rt_param_r->cmp_coeff, param->rtrans_cmp_coeff, rt_param_r->cmp_yorder * yorder_size);

        } else {
            rt_param_r->ff_yorder = param->rff_yorder;
            rt_param_r->fb_yorder = param->rfb_yorder;
            rt_param_r->cmp_yorder = param->rcmp_yorder;
            rt_param_r->ffgain = param->gains.r_ffgain;
            rt_param_r->fbgain = param->gains.r_fbgain;
            rt_param_r->cmpgain = param->gains.r_cmpgain;

            memcpy(rt_param_r->ff_coeff, param->rff_coeff, rt_param_r->ff_yorder * yorder_size);
            memcpy(rt_param_r->fb_coeff, param->rfb_coeff, rt_param_r->fb_yorder * yorder_size);
            memcpy(rt_param_r->cmp_coeff, param->rcmp_coeff, rt_param_r->cmp_yorder * yorder_size);
        }
        rt_param_r->ff_fade_gain = ((float)hdl->fade_ctr.rff_gain) / 16384.0f;
        rt_param_r->fb_fade_gain = ((float)hdl->fade_ctr.rfb_gain) / 16384.0f;
        rt_param_r->anc_mode = anc_mode;
        //printf(" ================== hdl->dut_mode %d\n",  hdl->dut_mode);
        rt_param_r->test_mode = hdl->dut_mode;
        rt_param_r->param = param;
    }
#endif

}

int audio_rtanc_var_cache_set(u8 flag)
{
    if (hdl) {
        rtanc_log("audio_rtanc:var_cache_flag %d\n", flag);
        hdl->var_cache_buff.flag = flag;
        if (!flag) {
            hdl->out.ff_use_outfgq = flag;
        }
    }
    return 0;
}

#if 0
struct __anc_ext_dynamic_cfg *dynamic_buffer;
struct __anc_ext_dynamic_cfg *anc_ext_dynamic_def_cfg_get()
{
    if (dynamic_buffer == NULL) {
        dynamic_buffer = (struct __anc_ext_dynamic_cfg *)zalloc(sizeof(struct __anc_ext_dynamic_cfg));
    }
    struct __anc_ext_dynamic_cfg *cfg = dynamic_buffer;

    cfg->sdcc_det_en = 1;
    cfg->sdcc_par1 = 0;
    cfg->sdcc_par2 = 4;
    cfg->sdcc_flag_thr = 1;
    cfg->sdrc_det_en = 1;
    cfg->sdrc_flag_thr = 1;
    cfg->sdrc_cnt_thr = 8;
    cfg->sdrc_ls_rls_lidx = 4;
    cfg->sdrc_ls_rls_hidx = 9;

    cfg->ref_iir_coef = 0.0078;
    cfg->err_iir_coef = 0.0078;
    cfg->sdcc_ref_thr = -24;
    cfg->sdcc_err_thr = -12;
    cfg->sdcc_thr_cmp = -24;
    cfg->sdrc_ref_thr = -24;
    cfg->sdrc_err_thr = -12;
    cfg->sdrc_ref_margin = 30;
    cfg->sdrc_err_margin = 35;
    cfg->sdrc_fb_att_thr = 0;
    cfg->sdrc_fb_rls_thr = -35;
    cfg->sdrc_fb_att_step = 2;
    cfg->sdrc_fb_rls_step = 2;
    cfg->sdrc_fb_set_thr = 3;
    cfg->sdrc_ff_att_thr = 0;
    cfg->sdrc_ff_rls_thr = -24;
    cfg->sdrc_ls_min_gain = 0;
    cfg->sdrc_ls_att_step = 6;
    cfg->sdrc_ls_rls_step = 2;
    cfg->sdrc_rls_ls_cmp = 15;
    return dynamic_buffer;
}

struct __anc_ext_rtanc_adaptive_cfg *rtanc_adpt_buffer;
struct __anc_ext_rtanc_adaptive_cfg *anc_ext_rtanc_adaptive_def_cfg_get()
{
    if (rtanc_adpt_buffer == NULL) {
        rtanc_adpt_buffer = (struct __anc_ext_rtanc_adaptive_cfg *)zalloc(sizeof(struct __anc_ext_rtanc_adaptive_cfg));
    }
    struct __anc_ext_rtanc_adaptive_cfg *cfg = rtanc_adpt_buffer;

    cfg->wind_det_en = 1;
    cfg->wind_lvl_thr = 30;
    cfg->wind_lock_cnt = 4;
    cfg->idx_use_same = 1;
    cfg->msc_dem_thr0 = 8;
    cfg->msc_dem_thr1 = 10;
    cfg->msc_mat_lidx = 5;
    cfg->msc_mat_hidx = 12;
    cfg->msc_idx_thr = 6;
    cfg->msc_scnt_thr = 2;
    cfg->msc2norm_updat_cnt = 5;
    cfg->msc_lock_cnt = 1;
    cfg->msc_tg_diff_lidx = 5;
    cfg->msc_tg_diff_hidx = 12;
    cfg->cmp_diff_lidx = 5;
    cfg->cmp_diff_hidx = 12;
    cfg->cmp_idx_thr = 4;
    cfg->cmp_adpt_en = 0;
    cfg->spl2norm_cnt = 5;
    cfg->talk_lock_cnt = 3;
    cfg->noise_idx_thr = 6;
    cfg->noise_updat_thr = 20;
    cfg->noise_ffdb_thr = 20;
    cfg->mse_lidx = 5;
    cfg->mse_hidx = 12;
    cfg->fast_cnt = 2;
    cfg->fast_ind_thr = 20;
    cfg->fast_ind_div = 2;
    cfg->norm_mat_lidx = 5;
    cfg->norm_mat_hidx = 12;
    cfg->tg_lidx = 5;
    cfg->tg_hidx = 12;
    cfg->rewear_idx_thr = 4;
    cfg->norm_updat_cnt0 = 10;
    cfg->norm_updat_cnt1 = 0;
    cfg->norm_cmp_cnt0 = 8;
    cfg->norm_cmp_cnt1 = 3;
    cfg->norm_cmp_lidx = 5;
    cfg->norm_cmp_hidx = 12;
    cfg->updat_lock_cnt = 2;
    cfg->ls_fix_mode = 1;
    cfg->ls_fix_lidx = 5;
    cfg->ls_fix_hidx = 10;
    cfg->iter_max0 = 20;
    cfg->iter_max1 = 20;
    cfg->m1_dem_cnt0 = 5;
    cfg->m1_dem_cnt1 = 4;
    cfg->m1_dem_cnt2 = 10;
    cfg->m1_scnt0 = 3;
    cfg->m1_scnt1 = 3;
    cfg->m1_dem_off_cnt0 = 10;
    cfg->m1_dem_off_cnt1 = 10;
    cfg->m1_dem_off_cnt2 = 10;
    cfg->m1_scnt0_off = 5;
    cfg->m1_scnt1_off = 5;
    cfg->m1_scnt2_off = 10;
    cfg->m1_idx_thr = 7;
    cfg->msc_iir_idx_thr = 5;
    cfg->msc_atg_sm_lidx = 9;
    cfg->msc_atg_sm_hidx = 24;
    cfg->msc_atg_diff_lidx = 10;
    cfg->msc_atg_diff_hidx = 25;
    cfg->msc_spl_idx_cut = 20;
    cfg->tight_divide = 0;
    cfg->tight_idx_thr = 40;
    cfg->tight_idx_diff = 4;

    cfg->wind_ref_thr = 100;
    cfg->wind_ref_max = 110;
    cfg->wind_ref_min = 12;
    cfg->wind_miu_div = 100;
    cfg->msc_err_thr0 = 4;
    cfg->msc_err_thr1 = 2;
    cfg->msc_tg_thr = 3;
    cfg->cmp_updat_thr = 1.5;
    cfg->cmp_updat_fast_thr = 2.5;
    cfg->cmp_mul_factor = 2;
    cfg->cmp_div_factor = 60;
    cfg->low_spl_thr = 0;
    cfg->splcnt_add_thr = 0;
    cfg->noise_mse_thr = 8;
    cfg->lms_err = 59;
    cfg->mse_thr1 = 5;
    cfg->mse_thr2 = 8;
    cfg->uscale = 0.8;
    cfg->uoffset = 0.1;
    cfg->u_pre_thr = 0.5;
    cfg->u_cur_thr = 0.15;
    cfg->u_first_thr = 0.4;
    cfg->norm_tg_thr = 1.5;
    cfg->norm_cmp_thr = 2;
    cfg->ls_fix_range0[0] = -10;
    cfg->ls_fix_range0[1] = 10;
    cfg->ls_fix_range1[0] = 0;
    cfg->ls_fix_range1[1] = 16;
    cfg->ls_fix_range2[0] = 12;
    cfg->ls_fix_range2[1] = 40;
    cfg->msc_tar_coef0 = 0.8;
    cfg->msc_ind_coef0 = 0.6;
    cfg->msc_tar_coef1 = 0.2;
    cfg->msc_ind_coef1 = 0.3;
    cfg->msc_atg_diff_thr0 = 4.0;
    cfg->msc_atg_diff_thr1 = 1.0;
    cfg->msc_tg_spl_thr = 4;
    cfg->msc_sz_sm_thr = 4;
    cfg->msc_atg_sm_thr = 3;

    return rtanc_adpt_buffer;
}
#endif

//设置RTANC初始参数
int audio_adt_rtanc_set_infmt(void *rtanc_tool)
{
    rtanc_debug_log("=================audio_adt_rtanc_set_infmt==================\n");
    if (++hdl->seq == 0xff) {
        hdl->seq = 0;
    }
    struct rt_anc_infmt infmt;
    if (anc_ext_tool_online_get()) {
        hdl->rtanc_tool = rtanc_tool;
    } else {
        hdl->rtanc_tool = NULL;
    }
    //查询是否有人挂起RTANC
    audio_rtanc_suspend_list_query(1);

#if AUDIO_RT_ANC_SZ_PZ_CMP_EN
    audio_rtanc_sz_pz_cmp_process();
#endif

    infmt.var_buf = &hdl->var_cache_buff;
    if (hdl->sz_sel.state & RTANC_SZ_STA_INIT) {
        hdl->sz_sel.state |= RTANC_SZ_STA_START;
        rtanc_log("RTANC_STATE_SZ:START\n");
        if (hdl->sz_sel.state == RTANC_SZ_STATE_TRIGGER) {
            os_taskq_post_msg("anc", 1, ANC_MSG_RTANC_SZ_OUTPUT);
            infmt.var_buf->cmp_ind_last = hdl->sz_sel.sel_idx - 1;
        }
    }
    infmt.id = hdl->seq;
    infmt.param = hdl->param;
    infmt.ext_cfg = (void *)anc_ext_ear_adaptive_cfg_get();
    infmt.anc_param_l = anc_malloc("ICSD_RTANC", sizeof(__rt_anc_param));
    //gali 可优化
    infmt.anc_param_r = anc_malloc("ICSD_RTANC", sizeof(__rt_anc_param));

    //rtanc_log("rtanc_init flag %d, ind_last %d, ff_use_outfgq %d\n", hdl->var_cache_buff.flag, hdl->var_cache_buff.ind_last, hdl->out.ff_use_outfgq);
    rtanc_log("rtanc_init flag %d, %d, %d, %d, %d, %d\n", infmt.var_buf->flag, infmt.var_buf->ind_last, infmt.var_buf->ls_gain_flag, \
              infmt.var_buf->sdrc_flag_fix_ls, infmt.var_buf->sdrc_flag_rls_ls, infmt.var_buf->cmp_ind_last);
    rtanc_log("part1 %d, %d, fb_gain %d\n", (int)(infmt.var_buf->part1_ref_lf_dB * 1000.0f), (int)(infmt.var_buf->part1_err_lf_dB * 1000.0f), \
              (int)(infmt.var_buf->fb_aim_gain * 1000.0f));
    rt_anc_get_param(infmt.anc_param_l, infmt.anc_param_r);

    struct __anc_ext_rtanc_adaptive_cfg *rtanc_tool_cfg = anc_ext_rtanc_adaptive_cfg_get();
    struct __anc_ext_dynamic_cfg *dynamic_cfg = anc_ext_dynamic_cfg_get();

    rt_anc_set_init(&infmt, rtanc_tool_cfg, dynamic_cfg);

    if (infmt.anc_param_l) {
        anc_free(infmt.anc_param_l);
    }
    if (infmt.anc_param_r) {
        anc_free(infmt.anc_param_r);
    }
    return 0;
}

static void audio_rt_anc_param_updata(void *rt_param_l, void *rt_param_r)
{
    if (anc_mode_get() != ANC_ON) {
        return;
    }
    rtanc_debug_log("audio_rt_anc_param_updata\n");

    audio_anc_t *param = hdl->param;
    u8 ff_updat = 0;
    u8 fb_updat = 0;
    u8 cmp_eq_updat = 0;

    __rt_anc_param *anc_param = (__rt_anc_param *)rt_param_l;

    //sz_sel 模式已经存有FF滤波器, 不需要拷贝
    if (hdl->sz_sel.state != RTANC_SZ_STATE_TRIGGER) {
        if (hdl->out.lff_coeff) {
            anc_free(hdl->out.lff_coeff);
        }
        hdl->out.lff_coeff = anc_malloc("ICSD_RTANC", param->lff_yorder * AUDIO_RTANC_COEFF_SIZE);
        hdl->out.ff_use_outfgq = 1;
        memcpy(hdl->out.lff_fgq, anc_param->ff_fgq, 31 * 4);
        memcpy(hdl->out.lff_coeff, anc_param->ff_coeff, param->lff_yorder * AUDIO_RTANC_COEFF_SIZE);
    }
    //put_buf((u8 *)hdl->out.lff_coeff, param->lff_yorder * AUDIO_RTANC_COEFF_SIZE);
    hdl->out.lff_gain = anc_param->ffgain;
    hdl->out.lfb_gain = anc_param->fbgain;
    param->gains.l_ffgain = anc_param->ffgain;
    param->lff_coeff = hdl->out.lff_coeff;
    param->gains.l_fbgain = anc_param->fbgain;
    ff_updat |= anc_param->ff_updat;
    fb_updat |= anc_param->fb_updat;
    cmp_eq_updat |= anc_param->cmp_eq_updat;

    //param->lfb_coeff = &anc_param->lfb_coeff[0];
#if ANC_CONFIG_RFB_EN
    if (rt_param_r) {
        __rt_anc_param *anc_param_r = (__rt_anc_param *)rt_param_r;
        if (hdl->out.rff_coeff) {
            anc_free(hdl->out.rff_coeff);
        }
        hdl->out.rff_coeff = anc_malloc("ICSD_RTANC", param->rff_yorder * AUDIO_RTANC_COEFF_SIZE);
        memcpy(hdl->out.rff_coeff, anc_param_r->ff_coeff, param->rff_yorder * AUDIO_RTANC_COEFF_SIZE);

        param->gains.r_ffgain = anc_param_r->ffgain;
        param->rff_coeff = hdl->out.rff_coeff;
        param->gains.r_fbgain = anc_param_r->fbgain;

        ff_updat |= anc_param_r->ff_updat;
        fb_updat |= anc_param_r->fb_updat;
        cmp_eq_updat |= anc_param_r->cmp_eq_updat;
    }
#endif

    //工具没有开CMP则不输出
    if (!anc_ext_adaptive_cmp_tool_en_get()) {
        cmp_eq_updat &= ~AUDIO_ADAPTIVE_CMP_UPDATE_FLAG;
    }
    if (ff_updat) {
        param->ff_filter_fade_slow = anc_param->ff_fade_slow;
        audio_anc_coeff_check_crc(ANC_CHECK_RTANC_UPDATE);
        anc_coeff_ff_online_update(hdl->param);		//更新ANC FF滤波器
    }
    //有CMP更新时，统一在CMP输出后更新FB
    if (fb_updat && (!(cmp_eq_updat & AUDIO_ADAPTIVE_CMP_UPDATE_FLAG))) {
        audio_anc_coeff_check_crc(ANC_CHECK_RTANC_UPDATE);
        anc_coeff_fb_online_update(hdl->param);		//更新ANC FB滤波器
    }
}

void audio_rtanc_cmp_update(void)
{
    audio_anc_t *param = hdl->param;

#if TCFG_AUDIO_ANC_ADAPTIVE_CMP_EN
    struct anc_cmp_param_output cmp_p;
    if (hdl->out.lcmp_coeff) {
        anc_free(hdl->out.lcmp_coeff);
    }
    hdl->out.lcmp_coeff = anc_malloc("ICSD_RTANC", ANC_ADAPTIVE_CMP_ORDER * AUDIO_RTANC_COEFF_SIZE);
    cmp_p.l_coeff = hdl->out.lcmp_coeff;

#if ANC_CONFIG_RFB_EN
    if (hdl->out.rcmp_coeff) {
        anc_free(hdl->out.lcmp_coeff);
    }
    hdl->out.rcmp_coeff = anc_malloc("ICSD_RTANC", ANC_ADAPTIVE_CMP_ORDER * AUDIO_RTANC_COEFF_SIZE);
    cmp_p.r_coeff = hdl->out.rcmp_coeff;
#endif

    audio_rtanc_adaptive_cmp_output_get(&cmp_p);
    hdl->out.lcmp_gain = cmp_p.l_gain;
    if (param->mode == ANC_TRANSPARENCY) {
        u8 sign = param->gains.gain_sign;
        param->ltrans_cmpgain = (sign & ANCL_CMP_SIGN) ? (0 - cmp_p.l_gain) : cmp_p.l_gain;
        param->ltrans_cmp_coeff = cmp_p.l_coeff;
    } else {
        param->gains.l_cmpgain = cmp_p.l_gain;
        param->lcmp_coeff = cmp_p.l_coeff;
    }

#if ANC_CONFIG_RFB_EN
    param->gains.r_cmpgain = cmp_p.r_gain;
    param->rcmp_coeff = cmp_p.r_coeff;
#endif
    //rtanc_debug_log("cmp updata gain = %d, coef = %d, ", (int)(cmp_p.l_gain * 100), (int)(cmp_p.l_coeff[0] * 100));
    rtanc_log("ANC_CHECK:CMP_UPDATE, crc %x-%d/100\n", CRC16(cmp_p.l_coeff, ANC_ADAPTIVE_CMP_ORDER * AUDIO_RTANC_COEFF_SIZE), (int)(cmp_p.l_gain * 100));
#endif
    if (param->mode != ANC_OFF) {
        audio_anc_coeff_fb_smooth_update();
    }
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
    //RTANC de 算法强制退出时，将不再输出output
    //gali 需考虑打断的逻辑
    if ((!icsd_adt_is_running()) && (hdl->sz_sel.state != RTANC_SZ_STATE_TRIGGER)) {
        return;
    }
    rtanc_log("RTANC_STATE:OUTPUT_RUN\n");
    hdl->run_busy = 1;
    //挂起RT_ANC
    audio_anc_real_time_adaptive_suspend("RTANC_OUTPUT");

    //整理SZ数据结构
    __afq_output output = {0};// = anc_malloc("ICSD_RTANC", sizeof(__afq_output));// = &temp;

    if (hdl->sz_sel.state != RTANC_SZ_STATE_TRIGGER) {
        output.sz_l = anc_malloc("ICSD_RTANC", sizeof(__afq_data));
        output.sz_l->len = 120;
        output.sz_l->out = anc_malloc("ICSD_RTANC", sizeof(float) * 120);
        icsd_rtanc_alg_get_sz(output.sz_l->out, 0); //0:L  1:R
        audio_rt_anc_param_updata(rt_param_l, rt_param_r);
        __rt_anc_param *anc_param = (__rt_anc_param *)rt_param_l;
        if (anc_param->cmp_eq_szidx != 100) {
            audio_afq_common_sz_sel_idx_set(anc_param->cmp_eq_szidx + 1);
        } else {
            audio_afq_common_sz_sel_idx_set(0);
        }
    } else { //sz_sel 模式已经存有SZ
        output = hdl->sz_sel.output;
        audio_afq_common_sz_sel_idx_set(hdl->sz_sel.sel_idx);
        //g_printf("rtanc_output sz\n");
        //put_buf((u8 *)output.sz_l->out, 120 * 4);
#if RTANC_SZ_SEL_FF_OUTPUT
        audio_rt_anc_param_updata(rt_param_l, rt_param_r);
#endif
    }
    if (!CRC16(output.sz_l->out, 120 * 4)) {
        ((__rt_anc_param *)rt_param_l)->cmp_eq_updat = 0;
        // rtanc_log("sz_output err\n");
    }
    //put_buf((u8 *)output.sz_l->out, 120 * 4);


#if AUDIO_RT_ANC_EXPORT_TOOL_DATA_DEBUG
    audio_anc_real_time_adaptive_data_packet(hdl->rtanc_tool);

#if TCFG_AUDIO_ANC_ADAPTIVE_CMP_EN
    //当RTANC没有输出CMP时，使用上一次的CMP结果输出给工具
    if (!(((__rt_anc_param *)rt_param_l)->cmp_eq_updat & AUDIO_ADAPTIVE_CMP_UPDATE_FLAG)) {
        audio_rtanc_cmp_tool_data_catch(hdl->cmp_tool_data, hdl->cmp_tool_data_len);
    }
#endif

#endif

    if (((__rt_anc_param *)rt_param_l)->cmp_eq_updat) {
        audio_afq_common_cmp_eq_update_set(((__rt_anc_param *)rt_param_l)->cmp_eq_updat);
#if TCFG_AUDIO_ANC_ADAPTIVE_CMP_EN
        if (anc_ext_adaptive_cmp_tool_en_get()) {
            audio_anc_real_time_adaptive_suspend("RTCMP");
        }
#endif

#if TCFG_AUDIO_ADAPTIVE_EQ_ENABLE
        //如果AEQ在运行就挂起RTANC
        if (audio_adaptive_eq_state_get() && anc_ext_adaptive_eq_tool_en_get()) {
            audio_anc_real_time_adaptive_suspend("AEQ");
        }
#endif
        //执行挂在到AFQ 的算法，如AEQ, 内部会挂起RTANC
        if (hdl->sz_sel.state == RTANC_SZ_STATE_TRIGGER) {
            audio_afq_common_output_post_msg(&output, AUDIO_AFQ_PRIORITY_HIGH);
        } else {
            audio_afq_common_output_post_msg(&output, AUDIO_AFQ_PRIORITY_NORMAL);
        }
    }

    if (hdl->sz_sel.state != RTANC_SZ_STATE_TRIGGER) {
        anc_free(output.sz_l->out);
        anc_free(output.sz_l);
    }
    //RTANC LIB 内部busy清0
    ((__rt_anc_param *)rt_param_l)->updat_busy = 0;

    //恢复RT_ANC
    audio_anc_real_time_adaptive_resume("RTANC_OUTPUT");

    audio_rtanc_in_ancoff_add_output_cnt(1);

    rtanc_log("RTANC_STATE:OUTPUT_RUN END\n");
    hdl->run_busy = 0;
}

static void audio_rtanc_suspend_list_query(u8 init_flag)
{
    //ADT切模式过程并且非初始化时，禁止切换
    if (get_icsd_adt_mode_switch_busy() && !init_flag) {
        if (hdl->state != RT_ANC_STATE_SUSPEND) {	//如挂起RTANC，则表示RTANC活动中，允许释放
            return;
        }
    }
    if (!audio_anc_real_time_adaptive_state_get()) {
        return;
    }

    os_mutex_pend(&hdl->mutex, 0);
    if (!list_empty(&hdl->suspend_head)) {
#if AUDIO_RT_ANC_SUSPEND_OBJECT_PRINTF
        if (init_flag) {
            rtanc_log("RTANC suspend object = {\n");
            struct rtanc_suspend_bulk *bulk;
            list_for_each_entry(bulk, &hdl->suspend_head, entry) {
                rtanc_log("    %s\n", bulk->name);
                audio_rtanc_spp_send_data(RTANC_SPP_CMD_SUSPEND_STATE, (u8 *)(bulk->name), strlen(bulk->name) + 1);
            }
            rtanc_log("}\n");
        }
#endif
        if (hdl->state == RT_ANC_STATE_OPEN) {
            rtanc_log("RTANC_STATE:SUSPEND\n");
            hdl->state = RT_ANC_STATE_SUSPEND;
            if (hdl->app_func_en) {
                anc_ext_algorithm_state_update(ANC_EXT_ALGO_RTANC, ANC_EXT_ALGO_STA_SUSPEND, 0);
            }
            icsd_adt_rtanc_suspend();
        }
    } else {
        if (hdl->state == RT_ANC_STATE_SUSPEND) {
            rtanc_log("RTANC_STATE:SUSPEND->OPEN\n");
            audio_rtanc_spp_send_data(RTANC_SPP_CMD_RESUME_STATE, NULL, 0);
            if (hdl->app_func_en) {
                anc_ext_algorithm_state_update(ANC_EXT_ALGO_RTANC, ANC_EXT_ALGO_STA_OPEN, 0);
            }
            hdl->state = RT_ANC_STATE_OPEN;
            icsd_adt_rtanc_resume();
        }
    }
    os_mutex_post(&hdl->mutex);
}

//RT ANC挂起接口, suspend/resume 的name 必须一致
void audio_anc_real_time_adaptive_suspend(const char *name)
{
    struct rtanc_suspend_bulk *bulk, *new_bulk;
    if (!name) {
        return;
    }

    if (cpu_in_irq()) {
        int msg[3] = {(int) audio_anc_real_time_adaptive_suspend, 1, (int)name};
        int ret = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
        return;
    }

    /* rtanc_log("%s, %s\n", __func__, name); */
    audio_rtanc_spp_send_data(RTANC_SPP_CMD_SUSPEND_STATE, (u8 *)name, strlen(name) + 1);
    os_mutex_pend(&hdl->mutex, 0);

    if (!list_empty(&hdl->suspend_head)) {
        list_for_each_entry(bulk, &hdl->suspend_head, entry) {
            if (!strcmp(name, bulk->name)) {
                //rtanc_log("same name %s\n", name);
                os_mutex_post(&hdl->mutex);
                return;	//同名称则返回
            }
        }
    }
    new_bulk = anc_malloc("ICSD_RTANC", sizeof(struct rtanc_suspend_bulk));
    new_bulk->name = name;

    list_add(&new_bulk->entry, &hdl->suspend_head);
    os_mutex_post(&hdl->mutex);
    audio_rtanc_suspend_list_query(0);
}

//RT ANC恢复接口
void audio_anc_real_time_adaptive_resume(const char *name)
{
    struct rtanc_suspend_bulk *bulk;
    struct rtanc_suspend_bulk *temp;
    if (!name) {
        return;
    }

    if (cpu_in_irq()) {
        int msg[3] = {(int) audio_anc_real_time_adaptive_resume, 1, (int)name};
        int ret = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
        return;
    }

    /* rtanc_log("%s, %s\n", __func__, name); */
    os_mutex_pend(&hdl->mutex, 0);

    if (list_empty(&hdl->suspend_head)) {
        os_mutex_post(&hdl->mutex);
        return;
    }
    list_for_each_entry_safe(bulk, temp, &hdl->suspend_head, entry) {
        if (!strcmp(bulk->name, name)) {
            list_del(&bulk->entry);
            anc_free(bulk);
            break;
        }
    }
    os_mutex_post(&hdl->mutex);
    audio_rtanc_suspend_list_query(0);
}

int audio_anc_real_time_adaptive_suspend_get(void)
{
    if (hdl) {
        if (!list_empty(&hdl->suspend_head)) {
            return 1;
        }
    }
    return 0;
}

//打开RTANC 前准备，sync_mode 是否TWS同步模式
int audio_rtanc_adaptive_init(u8 sync_mode)
{
    int ret;

    rtanc_debug_log("=================rt_anc_open==================\n");
    rtanc_debug_log("rtanc state %d\n", audio_anc_real_time_adaptive_state_get());
    ret = audio_rtanc_permit(sync_mode);
    if (ret) {
        rtanc_log("Err:rt_anc_open permit sync:%d, ret:%d\n", sync_mode, ret);
        audio_anc_debug_err_send_data(ANC_DEBUG_APP_RTANC, &ret, sizeof(int));
        return ret;
    }

    hdl->run_busy = 0;
    rtanc_log("RTANC_STATE:OPEN\n");
    hdl->state = RT_ANC_STATE_OPEN;
    clock_alloc("ANC_RT_ADAPTIVE", AUDIO_RT_ANC_CLOCK_ALLOC);
    audio_rt_anc_iir_type_get();	//获取工具配置IIR TYPE
#if AUDIO_RT_ANC_SZ_PZ_CMP_EN
    hdl->pz_cmp = anc_malloc("ICSD_RTANC", 50 * sizeof(float));
    hdl->sz_cmp = anc_malloc("ICSD_RTANC", 50 * sizeof(float));
#endif
    hdl->rtanc_p = anc_malloc("ICSD_RTANC", sizeof(struct rtanc_param));
#if TCFG_AUDIO_ANC_ADAPTIVE_CMP_EN
    audio_anc_ear_adaptive_cmp_open(CMP_FROM_RTANC);
#endif
    return 0;
}

int audio_rtanc_adaptive_exit(void)
{
    rtanc_debug_log("================rt_anc_close==================\n");
    if (audio_anc_real_time_adaptive_state_get() == 0) {
        return 1;
    }
    rtanc_log("RTANC_STATE:CLOSE\n");
    hdl->state = RT_ANC_STATE_CLOSE;
    clock_free("ANC_RT_ADAPTIVE");
    anc_free(hdl->rtanc_p);
    hdl->rtanc_p = NULL;

#if AUDIO_RT_ANC_SZ_PZ_CMP_EN
    anc_free(hdl->pz_cmp);
    anc_free(hdl->sz_cmp);
    hdl->pz_cmp = NULL;
    hdl->sz_cmp = NULL;
#endif

#if TCFG_AUDIO_ANC_ADAPTIVE_CMP_EN
    audio_anc_ear_adaptive_cmp_close();
#endif
    rtanc_log("rt_anc_close ok\n");
    return 0;
}

int audio_rtanc_adaptive_en(u8 en)
{
    int ret;
    rtanc_log("audio_rtanc_adaptive_en: %d\n", en);
    if (en) {
#if AUDIO_ANC_DATA_EXPORT_VIA_UART
        if (audio_anc_develop_is_running()) {
            printf("RTANC_STATE: ERR ANC_DEV OPEN NOW\n");
            return -1;
        }
#endif
        ret = audio_rtanc_permit(0);
        if (ret) {
            return ret;
        }
        audio_rtanc_var_cache_set(0);

        return audio_icsd_adt_sync_open(ADT_REAL_TIME_ADAPTIVE_ANC_MODE);
    } else {
        audio_icsd_adt_sync_close(ADT_REAL_TIME_ADAPTIVE_ANC_MODE, 0);
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
    int ret;
    hdl->app_func_en = 1;
    ret = audio_rtanc_adaptive_en(1);
    if (!ret) {
        anc_ext_algorithm_state_update(ANC_EXT_ALGO_RTANC, ANC_EXT_ALGO_STA_OPEN, 0);
    }
    return ret;
}

int audio_anc_real_time_adaptive_close(void)
{
    int ret;
    hdl->app_func_en = 0;
    ret = audio_rtanc_adaptive_en(0);

    anc_ext_algorithm_state_update(ANC_EXT_ALGO_RTANC, ANC_EXT_ALGO_STA_CLOSE, 0);

#if TCFG_AUDIO_ANC_ADAPTIVE_CMP_EN
    //app层关闭RTANC，清掉CMP工具上报参数
    audio_rtanc_cmp_data_clear();
#endif

#if AUDIO_RT_ANC_KEEP_LAST_PARAM == 1
    if (!(audio_anc_production_mode_get() || get_app_anctool_spp_connected_flag())) {
        return ret;
    }
#elif AUDIO_RT_ANC_KEEP_LAST_PARAM == 2
    return ret;
#endif
    //恢复默认ANC参数
    if ((!ret) && (anc_mode_get() == ANC_ON)) {
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
    if (!rtanc_tool) {
        return;
    }
    int len = rtanc_tool->h_len;
    int ff_yorder = hdl->param->lff_yorder;

    int ff_dat_len =  sizeof(anc_fr_t) * ff_yorder + 4;


    u8 *ff_dat, *rff_dat;

    if (hdl->sz_sel.state == RTANC_SZ_STATE_TRIGGER) {//SZ_SEL时，输出SZ相关滤波器
        memcpy(rtanc_tool->ff_fgq_l, hdl->sz_sel.lff_out, ((1 + 3 * RT_ANC_MAX_ORDER) * sizeof(float)));
        rtanc_tool->h_len = len = 25;
        memcpy(rtanc_tool->h_freq,  sz_sel_h_freq_test, 4 * len);
        for (int i = 0; i < len; i++) {
            rtanc_tool->sz_out_l[2 * i] = hdl->sz_sel.output.sz_l->out[2 * mem_list[i]];
            rtanc_tool->sz_out_l[2 * i + 1] = hdl->sz_sel.output.sz_l->out[2 * mem_list[i] + 1];
        }
    }

#if ANC_CONFIG_LFF_EN
    ff_dat = anc_malloc("ICSD_RTANC", ff_dat_len);
    //RTANC 增益没有输出符号，需要对齐工具符号
    rtanc_tool->ff_fgq_l[0] = (hdl->param->gains.gain_sign & ANCL_FF_SIGN) ? \
                              (0 - rtanc_tool->ff_fgq_l[0]) : rtanc_tool->ff_fgq_l[0];
    audio_anc_fr_format(ff_dat, rtanc_tool->ff_fgq_l, ff_yorder, hdl->lff_iir_type);
#endif/*ANC_CONFIG_LFF_EN*/
#if ANC_CONFIG_RFF_EN
    rff_dat = anc_malloc("ICSD_RTANC", ff_dat_len);
    //RTANC 增益没有输出符号，需要对齐工具符号
    rtanc_tool->ff_fgq_r[0] = (hdl->param->gains.gain_sign & ANCR_FF_SIGN) ? \
                              (0 - rtanc_tool->ff_fgq_r[0]) : rtanc_tool->ff_fgq_r[0];
    audio_anc_fr_format(rff_dat, rtanc_tool->ff_fgq_r, ff_yorder, hdl->rff_iir_type);
#endif/*ANC_CONFIG_RFF_EN*/

    rtanc_debug_log("-- len1: %d, len2: %d\n", rtanc_tool->h_len, rtanc_tool->h2_len);
    rtanc_debug_log("-- ff_yorder = %d\n", ff_yorder);
    /* 先统一申请空间，因为下面不知道什么情况下调用函数 anc_data_catch 时令参数 init_flag 为1 */
    rtanc_tool_data = anc_data_catch(rtanc_tool_data, NULL, 0, 0, 1);

#if TCFG_USER_TWS_ENABLE
    if (bt_tws_get_local_channel() == 'R') {
        rtanc_debug_log("RTANC export send tool_data, ch:R\n");
        if (hdl->param->developer_mode) {
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->h_freq, len * 4, ANC_R_ADAP_FRE, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->sz_out_l, len * 8, ANC_R_ADAP_SZPZ, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->pz_out_l, len * 8, ANC_R_ADAP_PZ, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->target_out_l, len * 8, ANC_R_ADAP_TARGET, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->target_out_cmp_l, len * 8, ANC_R_ADAP_TARGET_CMP, 0);
        }
        rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)ff_dat, ff_dat_len, ANC_R_FF_IIR, 0);  //R_ff

        rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->h2_freq, rtanc_tool->h2_len * 4, ANC_R_ADAP_FRE_2, 0);
        rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->mse_l, rtanc_tool->h2_len * 8, ANC_R_ADAP_MSE, 0);
    } else
#endif/*TCFG_USER_TWS_ENABLE*/
    {
        rtanc_debug_log("RTANC export send tool_data, ch:L\n");

        /* put_buf((u8 *)rtanc_tool->h_freq, len * 4); */
        /* put_buf((u8 *)rtanc_tool->sz_out_l, len * 8); */
        /* put_buf((u8 *)rtanc_tool->pz_out_l, len * 8); */
        /* put_buf((u8 *)rtanc_tool->target_out_l, len * 8); */
        /* put_buf((u8 *)rtanc_tool->target_out_cmp_l, len * 8); */
        /* put_buf((u8 *)rtanc_tool->h2_freq, rtanc_tool->h2_len * 4); */
        /* put_buf((u8 *)rtanc_tool->mse_l, rtanc_tool->h2_len * 8); */

        /* put_buf((u8 *)ff_dat, ff_dat_len);  //R_ff */
        /* put_buf((u8 *)cmp_dat, cmp_dat_len);  //R_cmp */

        if (hdl->param->developer_mode) {
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->h_freq, len * 4, ANC_L_ADAP_FRE, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->sz_out_l, len * 8, ANC_L_ADAP_SZPZ, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->pz_out_l, len * 8, ANC_L_ADAP_PZ, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->target_out_l, len * 8, ANC_L_ADAP_TARGET, 0);
            rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->target_out_cmp_l, len * 8, ANC_L_ADAP_TARGET_CMP, 0);
        }
        rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)ff_dat, ff_dat_len, ANC_L_FF_IIR, 0);  //R_ff

        rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->h2_freq, rtanc_tool->h2_len * 4, ANC_L_ADAP_FRE_2, 0);
        rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)rtanc_tool->mse_l, rtanc_tool->h2_len * 8, ANC_L_ADAP_MSE, 0);
    }

#if ANC_CONFIG_LFF_EN
    anc_free(ff_dat);
#endif/*ANC_CONFIG_LFF_EN*/
#if ANC_CONFIG_RFF_EN
    anc_free(rff_dat);
#endif/*ANC_CONFIG_RFF_EN*/

}

#if TCFG_AUDIO_ANC_ADAPTIVE_CMP_EN
//将CMP 工具数据拼接到RTANC
static void audio_rtanc_cmp_tool_data_catch(u8 *data, int len)
{
    if (!data) {
        return;
    }
#if TCFG_USER_TWS_ENABLE
    if (bt_tws_get_local_channel() == 'R') {
        rtanc_log("cmp export send tool_data, ch:R\n");
#if ANC_CONFIG_LFB_EN
        rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)data, len, ANC_R_CMP_IIR, 0);  //R_cmp
#endif
    } else
#endif/*TCFG_USER_TWS_ENABLE*/
    {
        rtanc_debug_log("cmp export send tool_data, ch:L\n");

        /* put_buf((u8 *)cmp_dat, cmp_dat_len);  //R_cmp */
#if ANC_CONFIG_LFB_EN
        rtanc_tool_data = anc_data_catch(rtanc_tool_data, (u8 *)data, len, ANC_L_CMP_IIR, 0);  //R_cmp
#endif
    }
}

void audio_rtanc_cmp_data_clear(void)
{
    if (hdl) {
        if (hdl->cmp_tool_data) {
            anc_free(hdl->cmp_tool_data);
            hdl->cmp_tool_data = NULL;
            hdl->cmp_tool_data_len = 0;
        }
    }
}

//格式化CMP工具数据
void audio_rtanc_cmp_data_packet(void)
{
    if (!hdl->rtanc_tool) {
        return;
    }
    int cmp_yorder = ANC_ADAPTIVE_CMP_ORDER;
    int cmp_dat_len =  sizeof(anc_fr_t) * cmp_yorder + 4;
#if ANC_CONFIG_LFB_EN
    float *lcmp_output = audio_anc_ear_adaptive_cmp_output_get(ANC_EAR_ADAPTIVE_CMP_CH_L);
    u8 *lcmp_type = audio_anc_ear_adaptive_cmp_type_get(ANC_EAR_ADAPTIVE_CMP_CH_L);
    u8 *cmp_dat = anc_malloc("ICSD_RTANC", cmp_dat_len);
    audio_anc_fr_format(cmp_dat, lcmp_output, cmp_yorder, lcmp_type);
#endif

#if ANC_CONFIG_RFB_EN
    float *rcmp_output = audio_anc_ear_adaptive_cmp_output_get(ANC_EAR_ADAPTIVE_CMP_CH_R);
    u8 *rcmp_type = audio_anc_ear_adaptive_cmp_type_get(ANC_EAR_ADAPTIVE_CMP_CH_R);
    u8 *rcmp_dat = anc_malloc("ICSD_RTANC", cmp_dat_len);
    audio_anc_fr_format(rcmp_dat, rcmp_output, cmp_yorder, rcmp_type);
#endif

    if (hdl->cmp_tool_data) {
        free(hdl->cmp_tool_data);
    }
    hdl->cmp_tool_data = anc_malloc("ICSD_CMP", cmp_dat_len);
    hdl->cmp_tool_data_len = cmp_dat_len;
    memcpy(hdl->cmp_tool_data, cmp_dat, cmp_dat_len);

    rtanc_debug_log("cmp_data_packet yorder = %d\n", cmp_yorder);

    audio_rtanc_cmp_tool_data_catch(cmp_dat, cmp_dat_len);

#if ANC_CONFIG_LFB_EN
    anc_free(cmp_dat);
#endif

#if ANC_CONFIG_RFB_EN
    anc_free(rcmp_dat);
#endif

}
#endif/*TCFG_AUDIO_ANC_ADAPTIVE_CMP_EN*/

#endif

int audio_anc_real_time_adaptive_tool_data_get(u8 **buf, u32 *len)
{
    if ((!audio_anc_real_time_adaptive_suspend_get()) && audio_anc_real_time_adaptive_state_get()) {
        rtanc_log("rtanc tool_data err: running! need suspend/close, return!\n");
        return -1;
    }
    if (rtanc_tool_data == NULL) {
        rtanc_log("rtanc tool_data err: packet is NULL, return!\n");
        return -1;
    }
    if (rtanc_tool_data->dat_len == 0) {
        rtanc_log("rtanc tool_data err: dat_len == 0\n");
        return -1;
    }
    rtanc_log("rtanc_tool_data->dat_len %d\n", rtanc_tool_data->dat_len);
    *buf = rtanc_tool_data->dat;
    *len = rtanc_tool_data->dat_len;
    return 0;
}

int audio_rtanc_fade_gain_suspend(struct rt_anc_fade_gain_ctr *fade_ctr)
{
    return 0; // gain no support
    rtanc_debug_log("%s, state %d\n", __func__, audio_anc_real_time_adaptive_state_get());
    if ((!audio_anc_real_time_adaptive_state_get()) || \
        get_icsd_adt_mode_switch_busy() || anc_mode_switch_lock_get()) {
        hdl->fade_ctr = *fade_ctr;
        return 0;
    }
    if (hdl) {
        if (memcmp(&hdl->fade_ctr, fade_ctr, sizeof(struct rt_anc_fade_gain_ctr))) {
            audio_anc_real_time_adaptive_suspend("ANC_FADE");

            if (hdl->rtanc_p) {
                hdl->rtanc_p->ffl_filter.fade_gain = ((float)fade_ctr->lff_gain) / 16384.0f;
                hdl->rtanc_p->fbl_filter.fade_gain = ((float)fade_ctr->lfb_gain) / 16384.0f;
                hdl->rtanc_p->ffr_filter.fade_gain = ((float)fade_ctr->rff_gain) / 16384.0f;
                hdl->rtanc_p->fbr_filter.fade_gain = ((float)fade_ctr->rfb_gain) / 16384.0f;

                icsd_adt_rtanc_fadegain_update(hdl->rtanc_p);
            }
            hdl->fade_ctr = *fade_ctr;
        }
    }
    return 0;
}

int audio_rtanc_fade_gain_resume(void)
{
    audio_anc_real_time_adaptive_resume("ANC_FADE");
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

int get_rtanc_mode()
{
    return hdl->rtanc_mode;
}
static u8 rtanc_reset_flag = 0;
u8 get_rtanc_reset_flag()
{
    return rtanc_reset_flag;
}
void set_rtanc_reset_flag(u8 flag)
{
    rtanc_reset_flag = flag;
}

REGISTER_LP_TARGET(RTANC_lp_target) = {
    .name       = "RTANC",
    .is_idle    = RTANC_idle_query,
};

#if AUDIO_RT_ANC_SZ_PZ_CMP_EN
static void audio_rtanc_sz_pz_cmp_process(void)
{
    struct sz_pz_cmp_cal_param p;

#if TCFG_SPEAKER_EQ_NODE_ENABLE
    hdl->spk_eq_tab_l.global_gain = spk_eq_read_global_gain(0);
    hdl->spk_eq_tab_l.global_gain = eq_db2mag(hdl->spk_eq_tab_l.global_gain);
    hdl->spk_eq_tab_l.seg_num = spk_eq_read_seg_l((u8 **)&hdl->spk_eq_tab_l.seg) / sizeof(struct eq_seg_info);
#endif

#if ANC_DUT_MIC_CMP_GAIN_ENABLE
    hdl->mic_cmp = &hdl->param->mic_cmp;
#endif

//FF FB已包含SPK信息，不需要单独spk的数据
#if 0//TCFG_SPEAKER_EQ_NODE_ENABLE
    p.spk_eq_tab = &hdl->spk_eq_tab_l;
#else
    p.spk_eq_tab = NULL;
#endif
    p.pz_cmp_out = hdl->pz_cmp;
    p.sz_cmp_out = hdl->sz_cmp;
#if ANC_DUT_MIC_CMP_GAIN_ENABLE
    p.ff_gain = hdl->mic_cmp->lff_gain;
    p.fb_gain = hdl->mic_cmp->lfb_gain;
#endif
    icsd_anc_v2_sz_pz_cmp_calculate(&p);
}

void audio_rtanc_self_talk_output(u8 flag)
{
    // printf("self talk flag: %d\n", flag);
    /* audio_rtanc_spp_send_data(RTANC_SPP_CMD_SELF_STATE, &flag, 1); */
}

static void audio_rtanc_spp_send_data(u8 cmd, u8 *buf, int len)
{
    audio_anc_debug_app_send_data(ANC_DEBUG_APP_RTANC, cmd, buf, len);
}

void audio_rtanc_dut_mode_set(u8 mode)
{
    if (hdl) {
        hdl->dut_mode = mode;
    }
}

float *audio_rtanc_pz_cmp_get(void)
{
    if (hdl) {
        return hdl->pz_cmp;
    }
    return NULL;
}

float *audio_rtanc_sz_cmp_get(void)
{
    if (hdl) {
        return hdl->sz_cmp;
    }
    return NULL;
}
#endif


//SZ_SEL 模式: AFQ输出回调，根据SZ筛选一条稳定的SZ，和FF滤波器，提供给CMP/AEQ计算
static void audio_rtanc_sz_select_afq_output_hdl(struct audio_afq_output *p)
{
    //g_printf("afq_output sz\n");
    //put_buf((u8 *)(p->sz_l.out), p->sz_l.len * 4);
    u8 sz_alloc = 0, pz_alloc = 0;

#if AUDIO_RT_ANC_SZ_PZ_CMP_EN
    if (!hdl->pz_cmp) {
        pz_alloc = 1;
        hdl->pz_cmp = anc_malloc("ICSD_RTANC", 50 * sizeof(float));
    }
    if (!hdl->sz_cmp) {
        sz_alloc = 1;
        hdl->sz_cmp = anc_malloc("ICSD_RTANC", 50 * sizeof(float));
    }
    audio_rtanc_sz_pz_cmp_process();
#endif
    __afq_output *output = &hdl->sz_sel.output;
    if (output->sz_l) {
        anc_free(output->sz_l->out);
        anc_free(output->sz_l);
    }
    //输出完毕后释放
    output->sz_l = anc_malloc("ICSD_RTANC", sizeof(__afq_data));
    output->sz_l->len = p->sz_l.len;
    output->sz_l->out = anc_malloc("ICSD_RTANC", sizeof(float) * output->sz_l->len);

    memcpy((u8 *)output->sz_l->out, (u8 *)p->sz_l.out, sizeof(float) * output->sz_l->len);

    hdl->sz_sel.sel_idx = icsd_anc_sz_select_from_memory(hdl->sz_sel.lff_out, output->sz_l->out, p->sz_l.msc, output->sz_l->len);

    //g_printf("sz_alogm_output sz\n");
    //put_buf((u8 *)(output->sz_l->out), p->sz_l.len * 4);

    if (pz_alloc) {
        anc_free(hdl->pz_cmp);
        hdl->pz_cmp = NULL;
    }
    if (sz_alloc) {
        anc_free(hdl->sz_cmp);
        hdl->sz_cmp = NULL;
    }

#if RTANC_SZ_SEL_FF_OUTPUT
    //FGQ 输出为IIR:start
    int lff_yorder = hdl->param->lff_yorder;

    if (hdl->out.lff_coeff) {
        anc_free(hdl->out.lff_coeff);
    }
    hdl->out.lff_coeff = anc_malloc("ICSD_RTANC", lff_yorder * AUDIO_RTANC_COEFF_SIZE);

    int ff_dat_len =  sizeof(anc_fr_t) * lff_yorder + 4;
    u8 *ff_dat = anc_malloc("ICSD_RTANC", ff_dat_len);
    int alogm = audio_anc_gains_alogm_get(ANC_FF_TYPE);
    audio_anc_fr_format(ff_dat, hdl->sz_sel.lff_out, lff_yorder, hdl->lff_iir_type);

    anc_fr_t *fr = (anc_fr_t *)(ff_dat + 4);
    /*
    g_printf("lff_out\n");
    printf("gain %d/1000\n", (int)((*((float *)ff_dat)) * 1000.0f));
    for (int i = 0; i < lff_yorder; i++) {
        printf("type %d, f %d, g %d, q %d\n", fr[i].type, \
               (int)(fr[i].a[0] * 1000.f), (int)(fr[i].a[1] * 1000.f), \
               (int)(fr[i].a[2] * 1000.f));
    }
    */
    audio_anc_biquad2ab_double((anc_fr_t *)(ff_dat + 4), hdl->out.lff_coeff, lff_yorder, alogm);

    hdl->out.lff_gain = hdl->sz_sel.lff_out[0] * RTANC_SZ_SEL_FF_OUTPUT_GAIN_RATIO;
    hdl->param->gains.l_ffgain = hdl->sz_sel.lff_out[0] * RTANC_SZ_SEL_FF_OUTPUT_GAIN_RATIO;
    hdl->param->lff_coeff = hdl->out.lff_coeff;
    rtanc_debug_log("sz_alogm_ff_coeff output\n");
    // put_buf((u8 *)hdl->out.lff_coeff, 10 * AUDIO_RTANC_COEFF_SIZE);
    anc_free(ff_dat);
    //FGQ 输出为AABB:stop
#endif

#if TCFG_AUDIO_ANC_ADAPTIVE_CMP_EN
    //挑线输出CMP滤波器并输出为IIR
    //FGQ 输出为IIR:start
    int lcmp_yorder = hdl->param->lcmp_yorder;
    u8 cmp_iir_type[ANC_ADAPTIVE_CMP_ORDER];
    hdl->sz_sel.lcmp_out = anc_malloc("ICSD_RTANC", (1 + 3 * RT_ANC_MAX_ORDER) * sizeof(float));

    audio_anc_ear_adaptive_cmp_sz_sel(hdl->sz_sel.sel_idx, hdl->sz_sel.lcmp_out, cmp_iir_type);
    if (hdl->out.lcmp_coeff) {
        anc_free(hdl->out.lcmp_coeff);
    }
    hdl->out.lcmp_coeff = anc_malloc("ICSD_RTANC", lcmp_yorder * AUDIO_RTANC_COEFF_SIZE);

    int cmp_dat_len =  sizeof(anc_fr_t) * lcmp_yorder + 4;
    u8 *cmp_dat = anc_malloc("ICSD_RTANC", cmp_dat_len);
    int cmp_alogm = audio_anc_gains_alogm_get(ANC_CMP_TYPE);
    audio_anc_fr_format(cmp_dat, hdl->sz_sel.lcmp_out, lcmp_yorder, cmp_iir_type);

    anc_fr_t *fr = (anc_fr_t *)(cmp_dat + 4);

    audio_anc_biquad2ab_double((anc_fr_t *)(cmp_dat + 4), hdl->out.lcmp_coeff, lcmp_yorder, cmp_alogm);

    hdl->out.lcmp_gain = hdl->sz_sel.lcmp_out[0];
    hdl->param->gains.l_cmpgain = hdl->sz_sel.lcmp_out[0];
    hdl->param->lcmp_coeff = hdl->out.lcmp_coeff;
    rtanc_debug_log("sz_alogm_cmp_coeff output\n");
    // put_buf((u8 *)hdl->out.lcmp_coeff, 10 * AUDIO_RTANC_COEFF_SIZE);
    anc_free(cmp_dat);
    free(hdl->sz_sel.lcmp_out);
    //FGQ 输出为AABB:stop
#endif

    hdl->sz_sel.state |= RTANC_SZ_STA_AFQ_OUT;
    rtanc_log("RTANC_STATE_SZ:AFQ_OUT\n");
    audio_afq_common_del_output_handler("RTANC");
}

//SZ_SEL 模式:启动
int audio_rtanc_sz_select_open(void)
{
    //选择数据来源AFQ
    if (hdl->sz_sel.state) { //重入判定
        return 1;
    }
    int fre_sel = AUDIO_ADAPTIVE_FRE_SEL_AFQ;
    hdl->sz_sel.state = RTANC_SZ_STA_INIT;
    if (hdl->sz_sel.lff_out == NULL) {
        hdl->sz_sel.lff_out = anc_malloc("ICSD_RTANC", (1 + 3 * RT_ANC_MAX_ORDER) * sizeof(float));
    }
    /* audio_anc_fade_ctr_set(ANC_FADE_MODE_RTANC_SZ_HOLD, AUDIO_ANC_FDAE_CH_ALL, 0); */
    rtanc_log("RTANC_STATE_SZ:INIT %lu \n", (1 + 3 * RT_ANC_MAX_ORDER) * sizeof(float));
    audio_rt_anc_iir_type_get();	//获取工具配置IIR TYPE
    audio_anc_real_time_adaptive_suspend("SZ_SEL");
    audio_afq_common_add_output_handler("RTANC", fre_sel, audio_rtanc_sz_select_afq_output_hdl);
    return 0;
}

/*
   SZ_SEL 模式: ANC线程处理, 更新RTANC效果，以及计算AEQ/CMP
   复位相关变量和资源
*/
int audio_rtanc_sz_select_process(void)
{
    __afq_output *output = &hdl->sz_sel.output;
    __rt_anc_param rt_param_l;

    rt_param_l.ffgain = hdl->out.lff_gain;
    rt_param_l.fbgain = hdl->param->gains.l_fbgain;
    rt_param_l.ff_updat = rt_param_l.fb_updat = rt_param_l.cmp_eq_updat = \
                          (AUDIO_ADAPTIVE_AEQ_UPDATE_FLAG | AUDIO_ADAPTIVE_CMP_UPDATE_FLAG);


    audio_adt_rtanc_output_handle(&rt_param_l, NULL);

    rtanc_log("RTANC_STATE_SZ:STOP\n");
    hdl->sz_sel.state = 0;

    if (output->sz_l) {
        anc_free(output->sz_l->out);
        anc_free(output->sz_l);
        output->sz_l = NULL;
    }
    if (hdl->sz_sel.lff_out) {
        anc_free(hdl->sz_sel.lff_out);
        hdl->sz_sel.lff_out = NULL;
    }
    /* audio_anc_fade_ctr_set(ANC_FADE_MODE_RTANC_SZ_HOLD, AUDIO_ANC_FDAE_CH_ALL, 16384); */
    audio_anc_real_time_adaptive_resume("SZ_SEL");

    return 0;
}

int audio_rtanc_anc_off_switch(void)
{
    //用于ANC_OFF下，开关RTANC（mode switch内部会有判断音乐播放相关逻辑）
    if (anc_mode_get() == ANC_OFF) {
        set_rtanc_reset_flag(1);//处理切相同模式
        anc_mode_switch(ANC_OFF, 0);
        set_rtanc_reset_flag(0);
    }
    return 0;
}

int audio_rtanc_sz_sel_callback(void)
{
    //处于ANC_OFF模式需要关闭RTANC
    return audio_rtanc_anc_off_switch();
}

int audio_rtanc_sz_sel_state_get(void)
{
    if (hdl) {
        return (hdl->sz_sel.state & RTANC_SZ_STA_AFQ_OUT);
    }
    return 0;
}

int audio_rtanc_coeff_set(void)
{

#if TCFG_AUDIO_ANC_ADAPTIVE_CMP_EN
    //工具连接时，切ANC默认清掉CMP工具上报参数
    audio_rtanc_cmp_data_clear();
#endif

#if AUDIO_RT_ANC_KEEP_LAST_PARAM
    if ((audio_anc_production_mode_get() || get_app_anctool_spp_connected_flag()) && \
        (AUDIO_RT_ANC_KEEP_LAST_PARAM == 1)) {
        return 0;
    }
    if (hdl) {
        audio_anc_t *param = hdl->param;
        struct audio_rt_anc_output *out = &hdl->out;
#if AUDIO_RT_ANC_KEEP_FF_COEFF   //是否保留上次RTANC FF结果
        if (out->lff_coeff) {
            param->gains.l_ffgain = out->lff_gain;
            param->lff_coeff = out->lff_coeff;
        }
        if (out->lfb_gain != 0.0f) {	//初始化为0
            param->gains.l_fbgain = out->lfb_gain;
        }
#elif RTANC_SZ_SEL_FF_OUTPUT
        if (hdl->sz_sel.state & RTANC_SZ_STA_INIT) {
            if (out->lff_coeff) {
                param->gains.l_ffgain = out->lff_gain;
                param->lff_coeff = out->lff_coeff;
            }
            if (out->lfb_gain != 0.0f) {	//初始化为0
                param->gains.l_fbgain = out->lfb_gain;
            }
        }
#endif
        if (out->lcmp_coeff) {
            if (param->mode == ANC_ON) {
                param->gains.l_cmpgain = out->lcmp_gain;
                param->lcmp_coeff = out->lcmp_coeff;
            } else { //通透
                u8 sign = param->gains.gain_sign;
                param->ltrans_cmpgain = (sign & ANCL_CMP_SIGN) ? (0 - out->lcmp_gain) : out->lcmp_gain;
                param->ltrans_cmp_coeff = out->lcmp_coeff;
            }
        }
    }
#endif
    audio_anc_coeff_check_crc(ANC_CHECK_RTANC_SAVE);
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
