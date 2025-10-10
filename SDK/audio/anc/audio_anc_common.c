#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_anc_common.data.bss")
#pragma data_seg(".audio_anc_common.data")
#pragma const_seg(".audio_anc_common.text.const")
#pragma code_seg(".audio_anc_common.text")
#endif

/*
 ****************************************************************
 *							AUDIO ANC COMMON
 * File  : audio_anc_comon_plug.c
 * By    :
 * Notes : 存放ANC共用流程
 *
 ****************************************************************
 */

#include "app_config.h"

#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#include "audio_anc_common_plug.h"
#include "esco_player.h"
#include "adc_file.h"
#include "bt_tws.h"

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
#include "icsd_adt.h"
#include "icsd_adt_app.h"
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

#if TCFG_AUDIO_SPEAK_TO_CHAT_ENABLE
#include "icsd_vdt_app.h"
#endif

#if ANC_MULT_ORDER_ENABLE
#include "audio_anc_mult_scene.h"
#endif/*ANC_MULT_ORDER_ENABLE*/


#if 1
#define anc_log	printf
#else
#define anc_log (...)
#endif


struct anc_common_t {
    u8 production_mode;
};

static struct anc_common_t common_hdl;
#define __this  (&common_hdl)

//ANC进入产测模式
int audio_anc_production_enter(void)
{
    if (__this->production_mode == 0) {
        anc_log("ANC in Production mode enter\n");
        __this->production_mode = 1;
        //挂起产测互斥功能

        //其他系列没有此接口，暂时保留
        /* audio_anc_drc_toggle_set(0); */
#if ANC_EAR_ADAPTIVE_EN
        //耳道自适应将ANC参数修改为默认参数
        if (audio_anc_coeff_mode_get()) {
            audio_anc_coeff_adaptive_set(0, 0);
        }
#endif

#if ANC_ADAPTIVE_EN
        //关闭场景自适应功能
        audio_anc_power_adaptive_suspend();
#endif

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
        //关闭风噪检测、智能免摘、广域点击
        audio_icsd_adt_scene_set(ADT_SCENE_PRODUCTION, 1);
        if (audio_icsd_adt_is_running()) {
            audio_icsd_adt_reset(ADT_SCENE_PRODUCTION);
        }
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

#if ANC_MUSIC_DYNAMIC_GAIN_EN
        //关闭ANC防破音
        audio_anc_music_dynamic_gain_suspend();
#endif
        //关闭啸叫抑制、DRC、自适应DCC等功能；
    }
    return 0;
}

//ANC退出产测模式
int audio_anc_production_exit(void)
{
    if (__this->production_mode == 1) {
        anc_log("ANC in Production mode exit\n");
        __this->production_mode = 0;
        //恢复产测互斥功能
    }
    return 0;
}

int audio_anc_production_mode_get(void)
{
    return __this->production_mode;
}

/*
	检查通话和ANC复用MIC 模拟增益是否一致
	param:is_phone_caller		0 非通话调用，1 通话调用
	note:如外部使用ADC模拟开关，可能导致MIC序号匹配错误，需屏蔽检查，人工对齐增益
*/
int audio_anc_mic_gain_check(u8 is_phone_caller)
{
    /* return 0;	//屏蔽检查 */

    struct adc_file_cfg *cfg;
    u8 anc_gain, phone_gain;
    //当前处于ANC_ON 且在通话中
    if (anc_mode_get() != ANC_OFF && (esco_player_runing() || is_phone_caller)) {
        cfg = audio_adc_file_get_cfg();
        if (cfg == NULL) {
            return 0;
        }
        for (int i = 0; i < AUDIO_ADC_MAX_NUM; i++) {
            if ((cfg->mic_en_map & BIT(i)) && audio_anc_mic_en_get(i)) {
                phone_gain = audio_adc_file_get_gain(i);
                anc_gain = audio_anc_mic_gain_get(i);
                ASSERT(phone_gain == anc_gain, "ERR! [mic%d_gain], esco %d != anc %d, please check MIC gain in the anc_gains.bin and the stream.bin \n", \
                       i, phone_gain, anc_gain);
            }
        }
    }
    return 0;
}

#if TCFG_USER_TWS_ENABLE

/*ANC模式同步(tws模式)*/
int anc_mode_sync(struct anc_tws_sync_info *info)
{
    anc_user_mode_set(info->user_anc_mode);
#if ANC_EAR_ADAPTIVE_EN
    anc_ear_adaptive_seq_set(info->ear_adaptive_seq);
#endif/*ANC_EAR_ADAPTIVE_EN*/

#if ANC_MULT_ORDER_ENABLE
    audio_anc_mult_scene_id_sync(info->multi_scene_id);
#endif/*ANC_MULT_ORDER_ENABLE*/
    if (info->anc_mode == anc_mode_get()) {
        return -1;
    }
    os_taskq_post_msg("anc", 2, ANC_MSG_MODE_SYNC, info->anc_mode);
    return 0;
}

#define TWS_FUNC_ID_ANC_SYNC    TWS_FUNC_ID('A', 'N', 'C', 'S')
static void bt_tws_anc_sync(void *_data, u16 len, bool rx)
{
    if (rx) {
        struct anc_tws_sync_info *info = (struct anc_tws_sync_info *)_data;
        anc_log("[slave]anc_sync\n");
        put_buf(_data, len);
        /*先同步adt的状态，然后在切anc里面跑同步adt的动作*/
#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
        audio_anc_icsd_adt_state_sync(info);
#else
        anc_mode_sync(info);
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
    }
}

REGISTER_TWS_FUNC_STUB(app_anc_sync_stub) = {
    .func_id = TWS_FUNC_ID_ANC_SYNC,
    .func    = bt_tws_anc_sync,
};

//TWS 回连ANC信息同步发送API
void bt_tws_sync_anc(void)
{
    struct anc_tws_sync_info info;
    info.user_anc_mode = anc_user_mode_get();
    info.anc_mode = anc_mode_get();
#if ANC_EAR_ADAPTIVE_EN
    info.ear_adaptive_seq = anc_ear_adaptive_seq_get();
#endif/*ANC_EAR_ADAPTIVE_EN*/
#if ANC_MULT_ORDER_ENABLE
    info.multi_scene_id = audio_anc_mult_scene_get();
#endif/*ANC_MULT_ORDER_ENABLE*/
#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
#if TCFG_AUDIO_SPEAK_TO_CHAT_ENABLE
    info.vdt_state = audio_speak_to_chat_is_trigger();
#endif
    info.app_adt_mode = icsd_adt_app_mode_get();
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
    anc_log("[master]bt_tws_sync_anc\n");
    put_buf((u8 *)&info, sizeof(struct anc_tws_sync_info));
    tws_api_send_data_to_slave(&info, sizeof(struct anc_tws_sync_info), TWS_FUNC_ID_ANC_SYNC);
}
#endif


#endif
