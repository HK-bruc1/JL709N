#ifndef __ICSD_ADT_APP_H_
#define __ICSD_ADT_APP_H_

#include "typedef.h"
#include "icsd_anc_user.h"
#include "asm/anc.h"

#define SPEAK_TO_CHAT_TASK_NAME     "speak_to_chat"

/*智能免摘检测到声音和退出通透是否播放提示音*/
#define SPEAK_TO_CHAT_PLAY_TONE_EN  1

/*智能免摘每次检测到声音的测试提示音*/
#define SPEAK_TO_CHAT_TEST_TONE_EN  0

/* 通过蓝牙spp发送风噪信息
 * 需要同时打开TCFG_BT_SUPPORT_SPP和APP_ONLINE_DEBUG*/
#define ICSD_ADT_WIND_INFO_SPP_DEBUG_EN  0

/* 通过蓝牙spp发送音量自适应得噪声大小信息
 * 需要同时打开TCFG_BT_SUPPORT_SPP和APP_ONLINE_DEBUG*/
#define ICSD_ADT_VOL_NOISE_LVL_SPP_DEBUG_EN  0

/*串口写卡导出MIC的数据，需要先打开宏AUDIO_DATA_EXPORT_VIA_UART*/
#define ICSD_ADT_MIC_DATA_EXPORT_EN  0

/*支持和其他功能共用ADC,需要在audio_config_def.h定义TCFG_AUDIO_ADC_ENABLE_ALL_DIGITAL_CH*/
#define ICSD_ADT_SHARE_ADC_ENABLE    0

enum {
    AUDIO_ADT_CLOSE = 0,    //关闭关闭
    AUDIO_ADT_OPEN,     //打开状态
    AUDIO_ADT_CHAT,   //免摘状态
};

enum {
    ANC_WIND_NOISE_LVL0 = 1,
    ANC_WIND_NOISE_LVL1,
    ANC_WIND_NOISE_LVL2,
    ANC_WIND_NOISE_LVL3,
    ANC_WIND_NOISE_LVL4,
    ANC_WIND_NOISE_LVL5,
    ANC_WIND_NOISE_LVL_MAX,
};

enum {
    WIND_AREA_TAP_DOUBLE_CLICK = 2,     //双击
    WIND_AREA_TAP_THIRD_CLICK,      //三击
    WIND_AREA_TAP_MULTIPLE_CLICK,   //大于3次多次连击
};

enum ICSD_ADT_MODE {
    ADT_MODE_CLOSE = 0,
    ADT_SPEAK_TO_CHAT_MODE = BIT(0), 			//智能免摘
    ADT_WIND_NOISE_DET_MODE = BIT(1),			//风噪检测
    ADT_WIDE_AREA_TAP_MODE = BIT(2), 			//广域点击
    ADT_ENVIRONMENT_DET_MODE = BIT(3), 			// 环境声检测
    ADT_REAL_TIME_ADAPTIVE_ANC_MODE = BIT(4), 	// RT_ANC
    ADT_EAR_IN_DETCET_MODE = BIT(5), 			// 入耳检测
    ADT_ENV_NOISE_DET_MODE = BIT(6), 			// 环境噪声检测
    ADT_REAL_TIME_ADAPTIVE_ANC_TIDY_MODE = BIT(7), 	// RT_ANC TIDY mode
    //ADT_ADAPTIVE_DCC_MODE = BIT(8),			//自适应DCC
    ADT_HOWLING_DET_MODE = BIT(9),				//啸叫检测
};

#define ADT_ALL_FUNCTION_ENABLE		ADT_SPEAK_TO_CHAT_MODE | ADT_WIDE_AREA_TAP_MODE | \
ADT_ENVIRONMENT_DET_MODE | ADT_REAL_TIME_ADAPTIVE_ANC_MODE

enum {
    /*任务消息*/
    ICSD_ADT_VOICE_STATE = 1,
    ICSD_ADT_WIND_LVL,
    ICSD_ADT_WAT_RESULT,
    ICSD_ADT_ENV_NOISE_LVL,
    ICSD_ADT_TONE_PLAY,
    SPEAK_TO_CHAT_TASK_KILL,
    ICSD_ANC_MODE_SWITCH,
    ICSD_ADT_STATE_SYNC,
    ICSD_ADT_EXPORT_DATA_WRITE,

    /*对耳信息同步*/
    SYNC_ICSD_ADT_VOICE_STATE,
    SYNC_ICSD_ADT_WIND_LVL_CMP,
    SYNC_ICSD_ADT_WAT_RESULT,
    SYNC_ICSD_ADT_WIND_LVL_RESULT,
    SYNC_ICSD_ADT_SUSPEND,
    SYNC_ICSD_ADT_OPEN,
    SYNC_ICSD_ADT_CLOSE,
    SYNC_ICSD_ADT_SET_ANC_FADE_GAIN,
    SYNC_ICSD_ADT_ENV_NOISE_LVL_CMP,
    SYNC_ICSD_ADT_ENV_NOISE_LVL_RESULT,
    SYNC_ICSD_ADT_RTANC_DRC_RESULT,
    SYNC_ICSD_ADT_RTANC_TONE_RESUME,
    SYNC_ICSD_ADT_TONE_ADAPTIVE_SYNC,
};

//ANC扩展功能场景支持定义
enum ICSD_ADT_SCENE {
    //-------USER SCENE------------
    ADT_SCENE_IDEL          = 0,
    ADT_SCENE_A2DP          = BIT(0),	//A2DP场景
    ADT_SCENE_ESCO          = BIT(1),	//ESCO场景
    ADT_SCENE_MIC_EFFECT    = BIT(2),	//MIC混响场景
    ADT_SCENE_TONE          = BIT(3),	//提示音场景(未启用)
    ADT_SCENE_ANC_OFF       = BIT(4),	//ANC_OFF场景
    ADT_SCENE_ANC_TRANS     = BIT(5),	//通透场景
    //用户定义场景在此添加

    //------SDK FIX SCENE 默认不支持ADT---------
    ADT_SCENE_AFQ           = BIT(16),
    ADT_SCENE_PRODUCTION    = BIT(17),
};

/*风噪输出等级:0~255*/
typedef struct {
    u16 lvl1_thr;   //阈值1
    u16 lvl2_thr;   //阈值2
    u16 lvl3_thr;   //阈值3
    u16 lvl4_thr;   //阈值4
    u16 lvl5_thr;   //阈值5
    u16 dithering_step; //消抖风噪等级间距
    u8 last_lvl;
    u8 cur_lvl;
} wind_lvl_det_t;

typedef struct {
    u8 last_lvl;//记录上一次风噪等级
    u8 cnt;
    u8 det_times;//连续性检测 时间 约等于 150ms * det_times
    s8 last_lvl_diff;//实时风噪档位与当前实际样机生效档位的档位差
} wind_info_t;

#define ANC_FADE_GAIN_MAX_FLOAT (16384.0f)
struct anc_fade_handle {
    int timer_ms; //配置定时器周期
    u8 fade_setp_flag; //配置0:增益，1：dB值
    int timer_id; //记录定时器id
    int cur_gain; //记录当前增益
    int fade_setp;//记录淡入淡出步进
    float fade_setp_dB;//记录淡入淡出步进,dB
    int target_gain; //记录目标增益
    u8 fade_gain_mode;//记录当前设置fade gain的模式

    int cur_gain_tmp;
    float fade_setp_dB_tmp;
};

u8 get_icsd_anc_wind_noise_lvl(wind_lvl_det_t *wind_lvl_det, u8 wind_lvl);
void audio_adt_wn_process_fade_timer(void *p);
int audio_anc_wind_noise_process_fade(wind_info_t *wind_info, u8 anc_wind_noise_lvl);
/*anc增益淡入淡出*/
int audio_anc_gain_fade_process(struct anc_fade_handle *hdl, enum anc_fade_mode_t mode, int target_gain, int fade_time_ms);

/*获取是否需要先开anc再开免摘的状态*/
u8 get_adt_open_in_anc_state();

void audio_anc_mode_switch_in_adt(u8 anc_mode);

/*打开声音检测*/
int audio_acoustic_detector_open();

/*关闭声音检测*/
int audio_acoustic_detector_close();


void audio_icsd_adt_resume();
void audio_icsd_adt_suspend();

/*获取adt的模式*/
u16 get_icsd_adt_mode();

/*同步tws配对时，同步adt的状态*/
void audio_anc_icsd_adt_state_sync(u8 *data);
void audio_icsd_adt_state_sync_done(u16 adt_mode, u8 speak_to_chat_state);

int anc_adt_init();

int audio_icsd_adt_open(u16 adt_mode);
int audio_icsd_adt_sync_open(u16 adt_mode);
/*打开所有模块*/
int audio_icsd_adt_open_all();

int audio_icsd_adt_sync_close(u16 adt_mode, u8 suspend);
int audio_icsd_adt_close(u32 scene, u8 run_sync, u16 adt_mode, u8 suspend);
int audio_icsd_adt_res_close(u16 adt_mode, u8 suspend);
/*关闭所有模块*/
int audio_icsd_adt_close_all();

u8 audio_icsd_adt_is_running();
void audio_icsd_adt_info_sync(u8 *data, int len);;
int audio_icsd_adt_open_permit(u16 adt_mode);

/*参数更新接口*/
int audio_acoustic_detector_updata();
void set_icsd_adt_param_updata();

/*获取免摘需要多少个mic*/
u8 get_icsd_adt_mic_num();

/*获取进入免摘前anc的模式*/
u8 get_icsd_adt_anc_mode();

/*记录adt切换trans的状态*/
u8 set_adt_switch_trans_state(u8 state);

/*获取adt切换trans的状态*/
u8 get_adt_switch_trans_state();

/*获取ADT模式切换是否繁忙*/
u8 get_icsd_adt_mode_switch_busy();

/*设置RTANC的模式，在下一次开 or 关生效*/
void icsd_adt_rt_anc_mode_set(int mode);

void audio_acoustic_detector_output_hdl(u8 voice_state, u8 wind_lvl, u8 wat_result);

/*设置ADT 通透模式下的FB/CMP 参数*/
void audio_icsd_adt_trans_fb_param_set(audio_anc_t *param);

//获取当前ADT是否运行
u8 icsd_adt_is_running(void);

/************************* 智能免摘相关接口 ***********************/
/*打开智能免摘*/
int audio_speak_to_chat_open();

/*关闭智能免摘*/
int audio_speak_to_chat_close();

void audio_speak_to_chat_demo();

/*设置免摘定时结束的时间，单位ms*/
int audio_speak_to_char_end_time_set(u16 time);

/*设置智能免摘检测的灵敏度*/
int audio_speak_to_chat_sensitivity_set(u8 sensitivity);

int audio_speak_to_chat_open_in_anc_done();

/*char 定时结束后从通透同步恢复anc on /anc off*/
void audio_speak_to_char_suspend(void);
void audio_speak_to_char_sync_suspend(void);

/*获取智能免摘是否打开*/
u8 audio_speak_to_chat_is_running();

/*获取智能免摘状态*/
u8 get_speak_to_chat_state();

/*检测到讲话状态同步*/
void set_speak_to_chat_voice_state(u8 state);
void audio_speak_to_chat_voice_state_sync(void);

/*智能免摘识别结果输出回调*/
void audio_speak_to_chat_output_handle(u8 voice_state);


/************************* 广域点击相关接口 ***********************/
/*打开广域点击*/
int audio_wat_click_open();

/*关闭广域点击*/
int audio_wat_click_close();

/*设置是否忽略广域点击
 * ingore: 1 忽略点击，0 响应点击
 * 忽略点击的时间，单位ms*/
int audio_wide_area_tap_ignore_flag_set(u8 ignore, u16 time);

/*广域点击开关demo*/
void audio_wat_click_demo();

/*广域点击事件处理*/
void audio_wat_area_tap_event_handle(u8 wat_result);

/*广域点击识别结果输出回调*/
void audio_wat_click_output_handle(u8 wat_result);

/************************* 风噪检测相关接口 ***********************/
/*打开风噪检测*/
int audio_icsd_wind_detect_open();

/*关闭风噪检测*/
int audio_icsd_wind_detect_close();

/*风噪检测开关*/
void audio_icsd_wind_detect_en(u8 en);

u8 get_audio_icsd_local_wind_lvl();

/*获取风噪等级*/
u8 get_audio_icsd_wind_lvl();

/*风噪检测开关demo*/
void audio_icsd_wind_detect_demo();

/*风噪检测处理*/
int audio_anc_wind_noise_process(u8 wind_lvl);

/*风噪检测识别结果输出回调*/
void audio_icsd_wind_detect_output_handle(u8 wind_lvl);

/*重置风噪检测的初始参数*/
void audio_anc_wind_noise_fade_param_reset(void);

/*设置风噪检测设置增益，带淡入淡出功能*/
void audio_anc_wind_noise_fade_gain_set(int fade_gain, int fade_time);

/*风噪检测开关*/
void audio_icsd_wind_detect_en(u8 en);


int audio_cvp_icsd_wind_det_open();
int audio_cvp_icsd_wind_det_close();
u8 get_cvp_icsd_wind_lvl_det_state();
void audio_cvp_icsd_wind_det_demo();
void audio_cvp_wind_lvl_output_handle(u8 wind_lvl);


/************************* 音量自适应相关接口 ***********************/
/*打开音量自适应*/
int audio_icsd_adaptive_vol_open();

/*关闭音量自适应*/
int audio_icsd_adaptive_vol_close();

/*音量自适应使用demo*/
void audio_icsd_adaptive_vol_demo();

/*音量偏移处理*/
void audio_icsd_adptive_vol_event_process(u8 spldb_iir);

/************************* 环境增益自适应相关接口 ***********************/
/*打开anc增益自适应*/
int audio_anc_env_adaptive_gain_open();

/*关闭anc增益自适应*/
int audio_anc_env_adaptive_gain_close();

/*anc增益自适应使用demo*/
void audio_anc_env_adaptive_gain_demo();

/*环境噪声处理*/
int audio_env_noise_event_process(u8 spldb_iir);

/*设置环境自适应增益，带淡入淡出功能*/
void audio_anc_env_adaptive_fade_gain_set(int fade_gain, int fade_time);

/*重置环境自适应增益的初始参数*/
void audio_anc_env_adaptive_fade_param_reset(void);


/************************* 啸叫检测相关接口 ***********************/
/*打开啸叫检测*/
int audio_anc_howling_detect_open();

/*关闭啸叫检测*/
int audio_anc_howling_detect_close();

/*啸叫检测使用demo*/
void audio_anc_howling_detect_demo();

/*啸叫检测结果回调*/
void audio_anc_howling_detect_output_handle(u8 result);

/*初始化啸叫检测资源*/
void icsd_anc_soft_howling_det_init();

/*退出啸叫检测资源*/
void icsd_anc_soft_howling_det_exit(void);

//DRC处理结果回调
void audio_rtanc_drc_output(u8 output, float gain, float fb_gain);

//DRC TWS标志获取
u8 audio_rtanc_drc_flag_get(float **gain);

void adt_open_in_anc_set(u8 state);
//非对耳同步打开
int audio_icsd_adt_no_sync_open(u16 adt_mode);

/*阻塞性 重启ADT算法*/
int audio_icsd_adt_reset(u32 scene);

int audio_icsd_adt_scene_set(enum ICSD_ADT_SCENE scene, u8 enable);
void icsd_adt_lock_init();
void icsd_adt_lock();
void icsd_adt_unlock();

extern void *get_anc_lfb_coeff();
extern void *get_anc_lff_coeff();
extern void *get_anc_ltrans_coeff();
extern void *get_anc_ltrans_fb_coeff();
extern float get_anc_gains_l_fbgain();
extern float get_anc_gains_l_ffgain();
extern float get_anc_gains_l_transgain();
extern float get_anc_gains_lfb_transgain();
extern u8 get_anc_l_fbyorder();
extern u8 get_anc_l_ffyorder();
extern u8 get_anc_l_transyorder();
extern u8 get_anc_lfb_transyorder();
extern u32 get_anc_gains_alogm();
extern u32 get_anc_gains_trans_alogm();
extern void icsd_adt_drc_output(u8 flag, float gain, float fb_gain);
extern u8 icsd_adt_drc_read(float **gain);

void audio_adt_rtanc_sync_tone_resume(void);

u8 audio_anc_env_adaptive_cur_lvl(void);

void audio_adt_tone_adaptive_sync(void);

int icsd_adt_app_mode_set(u16 adt_mode, u8 en);

u32 icsd_adt_app_mode_get(void);
#endif
