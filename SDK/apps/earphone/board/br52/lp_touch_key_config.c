#include "app_config.h"
#include "syscfg_id.h"
#include "key_driver.h"
#include "cpu/includes.h"
#include "system/init.h"
#include "asm/lp_touch_key_api.h"
#include "key/iokey.h"
#include "iokey_config.h"

#define LOG_TAG_CONST       LP_KEY
#define LOG_TAG             "[LP_KEY]"
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#if TCFG_LP_TOUCH_KEY_ENABLE

struct lpctmu_platform_data lpctmu_pdata;

static struct lp_touch_key_platform_data lp_touch_key_config;


void lp_touch_key_cfg_dump(void)
{
    log_debug("********** dump lp touch key config ***********\n");

    log_debug("ext_stop_ch_en              = %d", lpctmu_pdata.ext_stop_ch_en);
    log_debug("ext_stop_sel                = %d", lpctmu_pdata.ext_stop_sel);
    log_debug("sample_window_time          = %d", lpctmu_pdata.sample_window_time);
    log_debug("sample_scan_time            = %d", lpctmu_pdata.sample_scan_time);
    log_debug("lowpower_sample_scan_time   = %d", lpctmu_pdata.lowpower_sample_scan_time);
    log_debug("aim_vol_delta               = %d", lpctmu_pdata.aim_vol_delta);
    log_debug("aim_charge_khz              = %d", lpctmu_pdata.aim_charge_khz);
    for (u8 ch = 0; ch < LPCTMU_CHANNEL_SIZE; ch ++) {
        if (lpctmu_pdata.ch[ch].enable) {
            log_debug("ch[%d].enable                = %d", ch, lpctmu_pdata.ch[ch].enable);
            log_debug("ch[%d].wakeup_en             = %d", ch, lpctmu_pdata.ch[ch].wakeup_en);
        }
    }

    log_debug("slide_mode_en               = %d\n", lp_touch_key_config.slide_mode_en);
    log_debug("slide_mode_key_value        = %d\n", lp_touch_key_config.slide_mode_key_value);
    log_debug("ldo_wkp_reset               = %d\n", lp_touch_key_config.ldo_wkp_reset);
    log_debug("charge_online_reset         = %d\n", lp_touch_key_config.charge_online_reset);
    log_debug("charge_mode_keep_touch      = %d\n", lp_touch_key_config.charge_mode_keep_touch);
    log_debug("charge_enter_algo_reset     = %d\n", lp_touch_key_config.charge_enter_algo_reset);
    log_debug("charge_exit_algo_reset      = %d\n", lp_touch_key_config.charge_exit_algo_reset);

    log_debug("long_press_reset_enable     = %d\n", lp_touch_key_config.long_press_reset_enable);
    log_debug("long_press_reset_time       = %d\n", lp_touch_key_config.long_press_reset_time);
    log_debug("softoff_wakeup_time         = %d\n", lp_touch_key_config.softoff_wakeup_time);
    log_debug("short_click_check_time      = %d\n", lp_touch_key_config.short_click_check_time);
    log_debug("long_click_check_time       = %d\n", lp_touch_key_config.long_click_check_time);
    log_debug("hold_click_check_time       = %d\n", lp_touch_key_config.hold_click_check_time);
    log_debug("*lpctmu_cfg                 = %x\n", (u32)lp_touch_key_config.lpctmu_cfg);

    for (u8 ch = 0; ch < LPCTMU_CHANNEL_SIZE; ch ++) {
        if (lp_touch_key_config.key[ch].enable) {
            log_debug("key[%d].enable               = %d\n", ch, lp_touch_key_config.key[ch].enable);
            log_debug("key[%d].wakeup_en            = %d\n", ch, lp_touch_key_config.key[ch].wakeup_en);
            log_debug("key[%d].key_value            = %d\n", ch, lp_touch_key_config.key[ch].key_value);
            log_debug("key[%d].range_sensity        = %d\n", ch, lp_touch_key_config.key[ch].range_sensity);
            log_debug("key[%d].algo_range_min       = %d\n", ch, lp_touch_key_config.key[ch].algo_range_min);
            log_debug("key[%d].algo_range_max       = %d\n", ch, lp_touch_key_config.key[ch].algo_range_max);
            log_debug("key[%d].algo_cfg0            = %d\n", ch, lp_touch_key_config.key[ch].algo_cfg0);
            log_debug("key[%d].algo_cfg2            = %d\n", ch, lp_touch_key_config.key[ch].algo_cfg2);
        }
    }
#if TCFG_LP_EARTCH_KEY_ENABLE
    log_debug("eartch_en               	   = %d\n", lp_touch_key_config.eartch_en);
    log_debug("eartch_ch               	   = %d\n", lp_touch_key_config.eartch_ch);
    log_debug("eartch_ref_ch               = %d\n", lp_touch_key_config.eartch_ref_ch);
    log_debug("eartch_soft_inear_val       = %d\n", lp_touch_key_config.eartch_soft_inear_val);
    log_debug("eartch_soft_outear_val      = %d\n", lp_touch_key_config.eartch_soft_outear_val);
#endif
}

int board_lp_touch_key_config()
{
    printf("read lp touch cfg info !\n\n");

    memset((u8 *)&lp_touch_key_config, 0, sizeof(struct lp_touch_key_platform_data));
    memset((u8 *)&lpctmu_pdata, 0, sizeof(struct lpctmu_platform_data));


#if 1

    u8 cfg_buf[80];
    memset(cfg_buf, 0, sizeof(cfg_buf));
    int len = syscfg_read(CFG_LP_TOUCH_KEY_ID, cfg_buf, sizeof(cfg_buf));
    if (len < 0) {
        printf("read touch key cfg error !\n");
        return 1;
    }
    //put_buf(cfg_buf, sizeof(cfg_buf));


    lp_touch_key_config.lpctmu_cfg                  = &lpctmu_pdata;

    lpctmu_pdata.sample_window_time                 = 2;
    lpctmu_pdata.sample_scan_time                   = 20;
    lpctmu_pdata.lowpower_sample_scan_time          = 100;
    lpctmu_pdata.ext_stop_ch_en                     = 0;
    lpctmu_pdata.ext_stop_sel                       = 0;

    lpctmu_pdata.aim_vol_delta                      = cfg_buf[5] | (cfg_buf[6] << 8);
    lpctmu_pdata.aim_charge_khz                     = cfg_buf[7] | (cfg_buf[8] << 8);

    lp_touch_key_config.slide_mode_en               = cfg_buf[1];
    lp_touch_key_config.slide_mode_key_value        = uuid2keyValue(cfg_buf[2] | (cfg_buf[3] << 8));

    lp_touch_key_config.ldo_wkp_reset               = 1;
    lp_touch_key_config.charge_online_reset         = 1;
    lp_touch_key_config.charge_enter_algo_reset     = 0;
    lp_touch_key_config.charge_exit_algo_reset      = 1;

    lp_touch_key_config.charge_mode_keep_touch      = cfg_buf[10];
    lp_touch_key_config.long_press_reset_enable     = cfg_buf[11];
    lp_touch_key_config.long_press_reset_time       = cfg_buf[12] | (cfg_buf[13] << 8);
    lp_touch_key_config.softoff_wakeup_time         = cfg_buf[14] | (cfg_buf[15] << 8);

    lp_touch_key_config.short_click_check_time      = 500;
    lp_touch_key_config.long_click_check_time       = 2000;
    lp_touch_key_config.hold_click_check_time       = 200;

    u8 ch, offset;
    for (u8 i = 0; i < LPCTMU_CHANNEL_SIZE; i ++) {
        offset = 16 + i * 14;
        if (cfg_buf[offset] != 0x0d) {
            break;
        }
        offset += 1;
        ch = cfg_buf[offset];
        offset += 1;
        lp_touch_key_config.key[ch].key_value       = uuid2keyValue(cfg_buf[offset] | (cfg_buf[offset + 1] << 8));
        offset += 2;
        lp_touch_key_config.key[ch].wakeup_en       = cfg_buf[offset];
        offset += 1;
        lp_touch_key_config.key[ch].algo_range_max  = cfg_buf[offset] | (cfg_buf[offset + 1] << 8);
        offset += 2;
        lp_touch_key_config.key[ch].algo_cfg0       = cfg_buf[offset] | (cfg_buf[offset + 1] << 8);
        offset += 2;
        lp_touch_key_config.key[ch].algo_cfg1       = cfg_buf[offset] | (cfg_buf[offset + 1] << 8);
        offset += 2;
        lp_touch_key_config.key[ch].algo_cfg2       = cfg_buf[offset] | (cfg_buf[offset + 1] << 8);
        offset += 2;
        lp_touch_key_config.key[ch].range_sensity   = cfg_buf[offset];

        lp_touch_key_config.key[ch].enable          = 1;
        lp_touch_key_config.key[ch].algo_range_min  = 50;
        lpctmu_pdata.ch[ch].enable                  = 1;
        lpctmu_pdata.ch[ch].wakeup_en               = lp_touch_key_config.key[ch].wakeup_en;
    }

#if TCFG_LP_EARTCH_KEY_ENABLE
    memset(cfg_buf, 0, sizeof(cfg_buf));
    len = syscfg_read(CFG_LP_TOUCH_KEY_EARTCH_ID, cfg_buf, sizeof(cfg_buf));
    if (len < 0) {
        printf("read touch key eartch cfg error !\n");
        return 1;
    }
    //put_buf(cfg_buf, sizeof(cfg_buf));
    lp_touch_key_config.eartch_en                   = 1;
    lp_touch_key_config.eartch_ch                   = cfg_buf[1];
    lp_touch_key_config.eartch_ref_ch               = cfg_buf[2];
    lp_touch_key_config.eartch_soft_inear_val       = cfg_buf[4] | (cfg_buf[5] << 8);
    lp_touch_key_config.eartch_soft_outear_val      = cfg_buf[6] | (cfg_buf[7] << 8);
#endif

#else

    lp_touch_key_config.lpctmu_cfg                  = &lpctmu_pdata;

    lpctmu_pdata.sample_window_time                 = 2;
    lpctmu_pdata.sample_scan_time                   = 20;
    lpctmu_pdata.lowpower_sample_scan_time          = 100;
    lpctmu_pdata.ext_stop_ch_en                     = 0;
    lpctmu_pdata.ext_stop_sel                       = 0;
    lpctmu_pdata.aim_vol_delta                      = 800;
    lpctmu_pdata.aim_charge_khz                     = 2500;

    lp_touch_key_config.slide_mode_en               = 0;
    lp_touch_key_config.slide_mode_key_value        = 3;

    lp_touch_key_config.ldo_wkp_reset               = 1;
    lp_touch_key_config.charge_online_reset         = 1;
    lp_touch_key_config.charge_enter_algo_reset     = 0;
    lp_touch_key_config.charge_exit_algo_reset      = 1;
    lp_touch_key_config.charge_mode_keep_touch      = 0;

    lp_touch_key_config.long_press_reset_enable     = 1;
    lp_touch_key_config.long_press_reset_time       = 8000;
    lp_touch_key_config.softoff_wakeup_time         = 1000;

    lp_touch_key_config.short_click_check_time      = 500;
    lp_touch_key_config.long_click_check_time       = 2000;
    lp_touch_key_config.hold_click_check_time       = 200;

    lp_touch_key_config.key[0].enable               = 0;//PB0
    lp_touch_key_config.key[1].enable               = 1;//PB1
    lp_touch_key_config.key[2].enable               = 0;//PB2
    lp_touch_key_config.key[3].enable               = 0;//PB3
    lp_touch_key_config.key[4].enable               = 0;//PB4

    lp_touch_key_config.key[0].wakeup_en            = 0;//PB0
    lp_touch_key_config.key[1].wakeup_en            = 1;//PB1
    lp_touch_key_config.key[2].wakeup_en            = 0;//PB2
    lp_touch_key_config.key[3].wakeup_en            = 0;//PB3
    lp_touch_key_config.key[4].wakeup_en            = 0;//PB4

    lp_touch_key_config.key[0].key_value            = 0;
    lp_touch_key_config.key[1].key_value            = 0;
    lp_touch_key_config.key[2].key_value            = 0;
    lp_touch_key_config.key[3].key_value            = 0;
    lp_touch_key_config.key[4].key_value            = 0;

    for (u8 ch = 0; ch < LPCTMU_CHANNEL_SIZE; ch ++) {
        if (lp_touch_key_config.key[ch].enable == 0) {
            lpctmu_pdata.ch[ch].enable              = 0;
            lpctmu_pdata.ch[ch].wakeup_en           = 0;
            continue;
        }
        lp_touch_key_config.key[ch].range_sensity   = 7;
        lp_touch_key_config.key[ch].algo_range_min  = 50;
        lp_touch_key_config.key[ch].algo_range_max  = 500;
        lp_touch_key_config.key[ch].algo_cfg0       = 15;
        lp_touch_key_config.key[ch].algo_cfg1       = 20;
        lp_touch_key_config.key[ch].algo_cfg2       = 80;
        lpctmu_pdata.ch[ch].enable                  = 1;
        lpctmu_pdata.ch[ch].wakeup_en               = lp_touch_key_config.key[ch].wakeup_en;
    }

#if TCFG_LP_EARTCH_KEY_ENABLE
    lp_touch_key_config.eartch_en                   = 1;
    lp_touch_key_config.eartch_ch                   = 2;
    lp_touch_key_config.eartch_ref_ch               = 1;
    lp_touch_key_config.eartch_soft_inear_val       = 80;
    lp_touch_key_config.eartch_soft_outear_val      = 60;
#endif

#endif

    lp_touch_key_cfg_dump();

    lp_touch_key_init(&lp_touch_key_config);

    return 0;
}
platform_initcall(board_lp_touch_key_config);

#endif

