#ifndef _LP_TOUCH_KEY_API_
#define _LP_TOUCH_KEY_API_

#include "typedef.h"
#include "asm/lpctmu_hw.h"


enum CTMU_P2M_EVENT {
    CTMU_P2M_CH0_RES_EVENT = 0x50,
    CTMU_P2M_CH0_SHORT_KEY_EVENT,
    CTMU_P2M_CH0_LONG_KEY_EVENT,
    CTMU_P2M_CH0_HOLD_KEY_EVENT,
    CTMU_P2M_CH0_FALLING_EVENT,
    CTMU_P2M_CH0_RAISING_EVENT,

    CTMU_P2M_CH1_RES_EVENT = 0x58,
    CTMU_P2M_CH1_SHORT_KEY_EVENT,
    CTMU_P2M_CH1_LONG_KEY_EVENT,
    CTMU_P2M_CH1_HOLD_KEY_EVENT,
    CTMU_P2M_CH1_FALLING_EVENT,
    CTMU_P2M_CH1_RAISING_EVENT,

    CTMU_P2M_CH2_RES_EVENT = 0x60,
    CTMU_P2M_CH2_SHORT_KEY_EVENT,
    CTMU_P2M_CH2_LONG_KEY_EVENT,
    CTMU_P2M_CH2_HOLD_KEY_EVENT,
    CTMU_P2M_CH2_FALLING_EVENT,
    CTMU_P2M_CH2_RAISING_EVENT,

    CTMU_P2M_CH3_RES_EVENT = 0x68,
    CTMU_P2M_CH3_SHORT_KEY_EVENT,
    CTMU_P2M_CH3_LONG_KEY_EVENT,
    CTMU_P2M_CH3_HOLD_KEY_EVENT,
    CTMU_P2M_CH3_FALLING_EVENT,
    CTMU_P2M_CH3_RAISING_EVENT,

    CTMU_P2M_CH4_RES_EVENT = 0x70,
    CTMU_P2M_CH4_SHORT_KEY_EVENT,
    CTMU_P2M_CH4_LONG_KEY_EVENT,
    CTMU_P2M_CH4_HOLD_KEY_EVENT,
    CTMU_P2M_CH4_FALLING_EVENT,
    CTMU_P2M_CH4_RAISING_EVENT,

    CTMU_P2M_EARTCH_IN_EVENT = 0x78,
    CTMU_P2M_EARTCH_OUT_EVENT,
};


enum LP_TOUCH_SOFTOFF_MODE {
    LP_TOUCH_SOFTOFF_MODE_LEGACY  = 0, //普通关机
    LP_TOUCH_SOFTOFF_MODE_ADVANCE = 1, //带触摸关机
};

enum touch_key_type {
    TOUCH_KEY_NULL,
    TOUCH_KEY_SHORT_CLICK,
    TOUCH_KEY_LONG_CLICK,
    TOUCH_KEY_HOLD_CLICK,
};


enum {
    TOUCH_KEY_EVENT_SLIDE_UP,
    TOUCH_KEY_EVENT_SLIDE_DOWN,
    TOUCH_KEY_EVENT_SLIDE_LEFT,
    TOUCH_KEY_EVENT_SLIDE_RIGHT,
    TOUCH_KEY_EVENT_MAX,
};


struct lp_touch_key_cfg {
    u8 enable;
    u8 wakeup_en;
    u8 key_value;
    u8 range_sensity;
    u16 algo_range_min;
    u16 algo_range_max;
    u16 algo_cfg0;
    u16 algo_cfg1;
    u16 algo_cfg2;
};

struct lp_touch_key_platform_data {

    u8 eartch_en;
    u8 eartch_ch;
    u8 eartch_ref_ch;
    u8 eartch_inear_ok;
    u8 eartch_last_state;
    u8 eartch_trim_flag;
    u16 eartch_trim_value;
    u16 eartch_soft_inear_val;
    u16 eartch_soft_outear_val;


    u8 slide_mode_en;
    u8 slide_mode_key_value;

    u8 ldo_wkp_reset;
    u8 charge_online_reset;
    u8 charge_mode_keep_touch;
    u8 charge_enter_algo_reset;
    u8 charge_exit_algo_reset;

    u8 click_cnt[LPCTMU_CHANNEL_SIZE];
    u8 last_key[LPCTMU_CHANNEL_SIZE];

    u8 key_ch_msg_lock;
    u16 key_ch_msg_lock_timer;

    u16 short_timer[LPCTMU_CHANNEL_SIZE];
    u16 long_timer[LPCTMU_CHANNEL_SIZE];
    u16 hold_timer[LPCTMU_CHANNEL_SIZE];

    u16 short_click_check_time;
    u16 long_click_check_time;
    u16 hold_click_check_time;

    u8 long_press_reset_enable;
    u16 long_press_reset_time;
    u16 softoff_wakeup_time;

    struct lp_touch_key_cfg key[LPCTMU_CHANNEL_SIZE];

    struct lpctmu_platform_data *lpctmu_cfg;
};


u32 lp_touch_key_power_on_status();

u32 lp_touch_key_alog_range_display(u8 *display_buf);

void lp_touch_key_init(struct lp_touch_key_platform_data *config);

void lp_touch_key_disable(void);

void lp_touch_key_enable(void);

void lp_touch_key_charge_mode_enter();

void lp_touch_key_charge_mode_exit();


#endif

