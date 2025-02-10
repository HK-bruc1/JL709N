#ifndef _LPCTMU_HW_H_
#define _LPCTMU_HW_H_

#include "typedef.h"


#define LPCTMU_CHANNEL_SIZE         5

#define LPCTMU_ANA_CFG_ADAPTIVE     1


enum {
    LPCTMU_VH_065V,
    LPCTMU_VH_070V,
    LPCTMU_VH_075V,
    LPCTMU_VH_080V,
};

enum {
    LPCTMU_VL_020V,
    LPCTMU_VL_025V,
    LPCTMU_VL_030V,
    LPCTMU_VL_035V,
};

enum {
    LPCTMU_ISEL_036UA,
    LPCTMU_ISEL_072UA,
    LPCTMU_ISEL_108UA,
    LPCTMU_ISEL_144UA,
    LPCTMU_ISEL_180UA,
    LPCTMU_ISEL_216UA,
    LPCTMU_ISEL_252UA,
    LPCTMU_ISEL_288UA
};

enum {
    LPCTMU_CH0_PB0,
    LPCTMU_CH1_PB1,
    LPCTMU_CH2_PB2,
    LPCTMU_CH3_PB3,
    LPCTMU_CH4_PB4,
};

enum CTMU_M2P_CMD {
    REQUEST_LPCTMU_IRQ = 0x50,
    REQUEST_LPTMR_IRQ,
    RESET_IDENTIFY_ALGO,
    RESET_IDENTIFY_ALGO_WITH_TRIM,
    REQUEST_LPCTMU_RES_TRIM,
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

enum lpctmu_wakeup_cfg {
    LPCTMU_WAKEUP_DISABLE,
    LPCTMU_WAKEUP_EN_WITHOUT_CHARGE_ONLINE,
    LPCTMU_WAKEUP_EN_ALWAYS,
};


struct lpctmu_platform_data {
    u8 ext_stop_ch_en;
    u8 ext_stop_sel;
    u8 sample_window_time;              //采样窗口时间 ms
    u8 sample_scan_time;                //多久采样一次 ms
    u8 lowpower_sample_scan_time;       //软关机下多久采样一次 ms
    u16 aim_vol_delta;
    u16 aim_charge_khz;
};

struct lpctmu_config_data {
    u8 ch_num;
    u8 ch_list[LPCTMU_CHANNEL_SIZE];
    u8 ch_en;
    u8 ch_wkp_en;
    u8 softoff_wakeup_cfg;
    const struct lpctmu_platform_data *pdata;
};



#define LPCTMU_PLATFORM_DATA_BEGIN(data) \
    const struct lpctmu_platform_data data = {

#define LPCTMU_PLATFORM_DATA_END() \
    .ext_stop_ch_en = 0, \
    .ext_stop_sel = 0,\
    .sample_window_time = 2, \
    .sample_scan_time = 20, \
    .lowpower_sample_scan_time = 100, \
}


void lpctmu_send_m2p_cmd(enum CTMU_M2P_CMD cmd);

u32 lpctmu_get_cur_ch_by_idx(u32 ch_idx);

u32 lpctmu_get_idx_by_cur_ch(u32 cur_ch);

void lpctmu_set_ana_hv_level(u32 level);

u32 lpctmu_get_ana_hv_level(void);

void lpctmu_set_ana_cur_level(u32 ch, u32 cur_level);

u32 lpctmu_get_ana_cur_level(u32 ch);

void lpctmu_init(struct lpctmu_config_data *cfg_data);

void lpctmu_disable(void);

void lpctmu_enable(void);

u32 lpctmu_is_sf_keep(void);


#endif


