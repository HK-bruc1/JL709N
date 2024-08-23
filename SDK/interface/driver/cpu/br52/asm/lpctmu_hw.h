#ifndef _LPCTMU_HW_H_
#define _LPCTMU_HW_H_

#include "typedef.h"


#define LPCTMU_CHANNEL_SIZE  5


enum CTMU_M2P_CMD {
    REQUEST_LPCTMU_IRQ = 0x50,
    REQUEST_LPTMR_IRQ,
    RESET_IDENTIFY_ALGO,
};

enum bt_arb_wl2ext_act {
    RF_PLL_EN = 1,
    RF_PLL_RN,
    RF_RX_LDO,
    RF_RX_EN,
    RF_TX_LDO,
    RF_TX_EN,
    EF_RX_TX_EN_XOR,
};

enum lpctmu_ext_stop_sel {
    BT_SIG_ACT0,
    BT_SIG_ACT1,
    BT_SIG_ACT0_ACT1_XOR,
    BT_SIG_ACT0_ACT1_AND,
};

struct lpctmu_ch_cfg {
    u8 enable;
    u8 wakeup_en;
};

struct lpctmu_platform_data {
    u8 ext_stop_ch_en;
    u8 ext_stop_sel;
    u8 sample_window_time;              //采样窗口时间 ms
    u8 sample_scan_time;                //多久采样一次 ms
    u8 lowpower_sample_scan_time;       //软关机下多久采样一次 ms
    u16 aim_vol_delta;
    u16 aim_charge_khz;
    struct lpctmu_ch_cfg ch[LPCTMU_CHANNEL_SIZE];
};

#define LPCTMU_PLATFORM_DATA_BEGIN(data) \
    static struct lpctmu_platform_data data = {

#define LPCTMU_PLATFORM_DATA_END() \
};


void lpctmu_send_m2p_cmd(enum CTMU_M2P_CMD cmd);

void lpctmu_set_ana_hv_level(u8 level);

u8 lpctmu_get_ana_hv_level(void);

void lpctmu_set_ana_cur_level(u8 ch, u8 cur_level);

u8 lpctmu_get_ana_cur_level(u8 ch);

void lpctmu_init(struct lpctmu_platform_data *pdata);

void lpctmu_disable(void);

void lpctmu_enable(void);

u8 lpctmu_is_sf_keep(void);


#endif


