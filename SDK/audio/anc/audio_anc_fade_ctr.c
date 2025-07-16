#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_anc_fade_ctr.data.bss")
#pragma data_seg(".audio_anc_fade_ctr.data")
#pragma const_seg(".audio_anc_fade_ctr.text.const")
#pragma code_seg(".audio_anc_fade_ctr.text")
#endif

/*
 ****************************************************************
 *							AUDIO ANC
 * File  : audio_anc_fade_ctr.c
 * By    : Junhao
 * Notes : ANC fade gain管理，支持多场景并行使用
 *
 ****************************************************************
 */

#include "app_config.h"

//目前仅支持BR28、BR50; BR36暂未支持
#if TCFG_AUDIO_ANC_ENABLE && (!(defined CONFIG_CPU_BR36))

#include "audio_anc.h"
#include "audio_anc_fade_ctr.h"

#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE
#include "rt_anc_app.h"
#endif

#if 0
#define anc_fade_log	printf
#else
#define anc_fade_log(...)
#endif/*log_en*/


struct anc_fade_context {
    struct list_head entry;
    enum anc_fade_mode_t mode;
    u8 ch;
    u16 fade_gain;
};

struct anc_fade_ctrl {
    struct list_head head;
    spinlock_t lock;
#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE
    u16 rtanc_resume_id;
    u8 rtanc_suspend;
#endif
};

struct anc_fade_ctrl *anc_fade_hdl = NULL;

static void anc_fade_ctr_list_del(enum anc_fade_mode_t mode)
{
    struct anc_fade_context *ctx;
    if (list_empty(&anc_fade_hdl->head)) {
        return;
    }
    list_for_each_entry(ctx, &anc_fade_hdl->head, entry) {
        if (ctx->mode == mode) {
            goto __fade_del;
        }
    }
    return;
__fade_del:
    list_del(&ctx->entry);
    free(ctx);
}

static void anc_fade_ctr_list_add(enum anc_fade_mode_t mode, u8 ch, u16 gain)
{
    struct anc_fade_context *new_ctx = malloc(sizeof(struct anc_fade_context));
    new_ctx->mode = mode;
    //通道检测，有些芯片 fade_gain 不区分通道
    new_ctx->ch = anc_fade_ctr_ch_check(ch);
    new_ctx->fade_gain = gain;
    list_add_tail(&new_ctx->entry, &anc_fade_hdl->head);
}

static int anc_fade_ctr_update(enum anc_fade_mode_t mode, u8 ch, u16 gain)
{
    struct anc_fade_context *ctx;
    if (list_empty(&anc_fade_hdl->head)) {
        return 1;
    }
    list_for_each_entry(ctx, &anc_fade_hdl->head, entry) {
        if (ctx->mode == mode) {
            if (ctx->ch != ch) {
                printf("ERR!!!anc_fade_gain mode&ch is no \"one to one\" \n");
            }
            ctx->fade_gain = gain;
            return 0;
        }
    }
    return 1;
}

void audio_anc_fade_ctr_init(void)
{
    anc_fade_hdl = zalloc(sizeof(struct anc_fade_ctrl));
    INIT_LIST_HEAD(&anc_fade_hdl->head);
    spin_lock_init(&anc_fade_hdl->lock);
}

#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE
static void audio_anc_fade_rtanc_resume_timeout(void *priv)
{
    anc_fade_hdl->rtanc_resume_id = 0;
    audio_anc_real_time_adaptive_resume("ANC_FADE");
}
#endif

static void audio_anc_fade_ctr_set_base(enum anc_fade_mode_t mode, u8 ch, u16 gain)
{
    u16 lff_gain, lfb_gain, rff_gain, rfb_gain;
    struct anc_fade_context *ctx;
    u8 fade_en = anc_api_get_fade_en();
    anc_fade_log("fade in mode %d, gain %d\n", mode, gain);

    lff_gain = lfb_gain = rff_gain = rfb_gain = AUDIO_ANC_FADE_GAIN_DEFAULT;

    spin_lock(&anc_fade_hdl->lock);
    if (gain == AUDIO_ANC_FADE_GAIN_DEFAULT) {
        anc_fade_ctr_list_del(mode);					//删除
    } else {
        if (anc_fade_ctr_update(mode, ch, gain)) {		//更新
            anc_fade_ctr_list_add(mode, ch, gain);		//新增
        }
    }

    //遍历链表
    if (!list_empty(&anc_fade_hdl->head)) {
        list_for_each_entry(ctx, &anc_fade_hdl->head, entry) {
            anc_fade_log("fade list : mode %d, gain %d\n", ctx->mode, ctx->fade_gain);
            if ((ctx->ch & AUDIO_ANC_FADE_CH_LFF) && (ctx->fade_gain < lff_gain)) {
                lff_gain = ctx->fade_gain;
            }
            if ((ctx->ch & AUDIO_ANC_FADE_CH_LFB) && (ctx->fade_gain < lfb_gain)) {
                lfb_gain = ctx->fade_gain;
            }
            if ((ctx->ch & AUDIO_ANC_FADE_CH_RFF) && (ctx->fade_gain < rff_gain)) {
                rff_gain = ctx->fade_gain;
            }
            if ((ctx->ch & AUDIO_ANC_FADE_CH_RFB) && (ctx->fade_gain < rfb_gain)) {
                rfb_gain = ctx->fade_gain;
            }
        }
    }
    spin_unlock(&anc_fade_hdl->lock);

#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE
    //当增益不等于16384 且 非ANC_OFF模式时，需挂起RTANC
    if ((lff_gain != AUDIO_ANC_FADE_GAIN_DEFAULT || lfb_gain != AUDIO_ANC_FADE_GAIN_DEFAULT || \
         rff_gain != AUDIO_ANC_FADE_GAIN_DEFAULT || rfb_gain != AUDIO_ANC_FADE_GAIN_DEFAULT) && \
        (!anc_fade_hdl->rtanc_suspend && anc_mode_get() != ANC_OFF)) {
        anc_fade_hdl->rtanc_suspend = 1;
        if (anc_fade_hdl->rtanc_resume_id) {
            sys_timer_del(anc_fade_hdl->rtanc_resume_id);
            anc_fade_hdl->rtanc_resume_id = 0;
        }
        audio_anc_real_time_adaptive_suspend("ANC_FADE");

    }
#endif

    anc_fade_log("fade lff %d, lfb %d, rff %d, rfb %d\n", lff_gain, lfb_gain, rff_gain, rfb_gain);
    /* if (mode == ANC_FADE_MODE_WIND_NOISE) { */
    /* audio_anc_fade_cfg_set(fade_en, 1, 15); */
    /* } else if (mode == ANC_FADE_MODE_HOWLDET) { */
    /* if (gain == 0) { */
    /* audio_anc_fade_cfg_set(fade_en, 14, 0); */
    /* } else { */
    /* audio_anc_fade_cfg_set(fade_en, 1, 15); */
    /* } */
    /* } else { */
    /* audio_anc_fade_cfg_set(fade_en, 1, 0); */
    /* } */

    audio_anc_fade_cfg_set(fade_en, 1, 0);

    audio_anc_fade(AUDIO_ANC_FADE_CH_LFF, lff_gain);
    audio_anc_fade(AUDIO_ANC_FADE_CH_LFB, lfb_gain);
    audio_anc_fade(AUDIO_ANC_FADE_CH_RFF, rff_gain);
    audio_anc_fade(AUDIO_ANC_FADE_CH_RFB, rfb_gain);

#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE
    //当增益等于16384 或者 ANC_OFF模式时，需恢复RTANC
    if ((lff_gain == AUDIO_ANC_FADE_GAIN_DEFAULT && lfb_gain == AUDIO_ANC_FADE_GAIN_DEFAULT && \
         rff_gain == AUDIO_ANC_FADE_GAIN_DEFAULT && rfb_gain == AUDIO_ANC_FADE_GAIN_DEFAULT && \
         anc_fade_hdl->rtanc_suspend) || anc_mode_get() == ANC_OFF) {
        anc_fade_hdl->rtanc_suspend = 0;
        if (anc_fade_hdl->rtanc_resume_id) {
            sys_timer_del(anc_fade_hdl->rtanc_resume_id);
            anc_fade_hdl->rtanc_resume_id = 0;
        }
        anc_fade_hdl->rtanc_resume_id = sys_timeout_add(NULL, audio_anc_fade_rtanc_resume_timeout, 60);
    }
#endif
}

void audio_anc_fade_ctr_set(enum anc_fade_mode_t mode, u8 ch, u16 gain)
{
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    if ((mode != ANC_FADE_MODE_WIND_NOISE) && \
        (mode != ANC_FADE_MODE_ENV_ADAPTIVE_GAIN) && \
        (mode != ANC_FADE_MODE_HOWLDET)) {
        printf("[anc_fade]: mode %d, gain %d, rets %x\n", mode, gain, rets_addr);
    }
    audio_anc_fade_ctr_set_base(mode, ch, gain);
}

void audio_anc_fade_ctr_del(enum anc_fade_mode_t mode)
{
    audio_anc_fade_ctr_set(mode, 0, AUDIO_ANC_FADE_GAIN_DEFAULT);
}

u16 audio_anc_fade_ctr_get(u8 mode, u8 ch)
{
    u16 lff_gain, lfb_gain, rff_gain, rfb_gain;
    struct anc_fade_context *ctx;
    r_printf("audio_anc_fade_ctr_get:\n");
    lff_gain = lfb_gain = rff_gain = rfb_gain = AUDIO_ANC_FADE_GAIN_DEFAULT;

    spin_lock(&anc_fade_hdl->lock);
    //遍历链表
    if (!list_empty(&anc_fade_hdl->head)) {
        list_for_each_entry(ctx, &anc_fade_hdl->head, entry) {
            // anc_fade_log("fade list : mode %d, gain %d\n", ctx->mode, ctx->fade_gain);
            if (mode == ctx->mode) {
                if ((ctx->ch & AUDIO_ANC_FADE_CH_LFF) && (ctx->fade_gain < lff_gain)) {
                    lff_gain = ctx->fade_gain;
                }
                if ((ctx->ch & AUDIO_ANC_FADE_CH_LFB) && (ctx->fade_gain < lfb_gain)) {
                    lfb_gain = ctx->fade_gain;
                }
                if ((ctx->ch & AUDIO_ANC_FADE_CH_RFF) && (ctx->fade_gain < rff_gain)) {
                    rff_gain = ctx->fade_gain;
                }
                if ((ctx->ch & AUDIO_ANC_FADE_CH_RFB) && (ctx->fade_gain < rfb_gain)) {
                    rfb_gain = ctx->fade_gain;
                }
                break;
            }
        }
    }
    spin_unlock(&anc_fade_hdl->lock);
    anc_fade_log("mode %d, fade lff %d, lfb %d, rff %d, rfb %d\n", mode, lff_gain, lfb_gain, rff_gain, rfb_gain);
    if (ch & AUDIO_ANC_FDAE_CH_ALL) {
        return lff_gain;
    } else if (ch & AUDIO_ANC_FDAE_CH_FF) {
        return lff_gain;
    } else if (ch & AUDIO_ANC_FDAE_CH_FB) {
        return lfb_gain;
    }
    return lff_gain;
}

u16 audio_anc_fade_ctr_get_min()
{
    u16 lff_gain, lfb_gain, rff_gain, rfb_gain;
    struct anc_fade_context *ctx;
    //r_printf("audio_anc_fade_ctr_get_min:\n");
    u8 mode = 0;
    lff_gain = lfb_gain = rff_gain = rfb_gain = AUDIO_ANC_FADE_GAIN_DEFAULT;
    spin_lock(&anc_fade_hdl->lock);
    //遍历链表
    if (!list_empty(&anc_fade_hdl->head)) {
        list_for_each_entry(ctx, &anc_fade_hdl->head, entry) {
            // anc_fade_log("fade list : mode %d, gain %d\n", ctx->mode, ctx->fade_gain);
            if ((ctx->ch & AUDIO_ANC_FADE_CH_LFF) && (ctx->fade_gain < lff_gain)) {
                mode = ctx->mode;
                lff_gain = ctx->fade_gain;
            }
            if ((ctx->ch & AUDIO_ANC_FADE_CH_LFB) && (ctx->fade_gain < lfb_gain)) {
                lfb_gain = ctx->fade_gain;
            }
            if ((ctx->ch & AUDIO_ANC_FADE_CH_RFF) && (ctx->fade_gain < rff_gain)) {
                rff_gain = ctx->fade_gain;
            }
            if ((ctx->ch & AUDIO_ANC_FADE_CH_RFB) && (ctx->fade_gain < rfb_gain)) {
                rfb_gain = ctx->fade_gain;
            }
        }
    }
    spin_unlock(&anc_fade_hdl->lock);
    printf("mode %d, fade lff %d, lfb %d\n", mode, lff_gain, lfb_gain);

    return lff_gain;
}

#endif/*TCFG_AUDIO_ANC_ENABLE*/

