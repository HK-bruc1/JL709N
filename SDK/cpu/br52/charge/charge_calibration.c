#include "uart.h"
#include "device/device.h"
#include "asm/power_interface.h"
#include "system/event.h"
#include "asm/efuse.h"
#include "gpio.h"
#include "app_config.h"
#include "asm/charge_calibration.h"
#include "asm/charge.h"
#include "system/includes.h"

#if TCFG_CHARGE_CALIBRATION_ENABLE

#define LOG_TAG_CONST   CHARGE
#define LOG_TAG         "[CHARGE]"
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"


//配置默认的校准区间范围在 (TCFG_CHARGE_MA + MAX_CUR_OFFSET) ~ (TCFG_CHARGE_MA - MIN_CUR_OFFSET)
#define MAX_CUR_OFFSET      5
#define MIN_CUR_OFFSET      5

static u32 calc_max_cur, calc_min_cur, now_current;
struct calibration_data {
    u32 cur_curr;//当前电流
    u32 adjust_curr;//调整后预想的电流值
};
static struct calibration_data cali_data;
static u16 config_charge_current = TCFG_CHARGE_MA * 100;
void charge_enter_calibration_mode(void)
{
    calibration_result cali_result;
    int ret = syscfg_read(VM_CHARGE_CALIBRATION, (void *)&cali_result, sizeof(calibration_result));
    if (ret == sizeof(calibration_result)) {
        log_info("calibration succ\n");
        set_charge_mA(cali_result.cali_current);
    } else {
        log_info("not calibration\n");
    }
}
static u8 charge_calibration_bigger(u32 curr_err)
{
    u8 ret = CALIBRATION_CONTINUE;
    cali_data.cur_curr = get_charge_current_value() * 100;
    cali_data.adjust_curr = (cali_data.cur_curr - curr_err) / 100;

    if ((cali_data.adjust_curr > CHARGE_mA_MAX) || (p.cur_curr < curr_err)) {
        log_info("Set current out of adjustment range");
        ret = CALIBRATION_MA_ERR;
    } else {
        set_charge_mA(cali_data.adjust_curr);
    }
    return ret;
}

static u8 charge_calibration_smaller(u32 curr_err)
{
    u8 ret = CALIBRATION_CONTINUE;
    cali_data.cur_curr = get_charge_current_value() * 100;
    cali_data.adjust_curr = (cali_data.cur_curr + curr_err) / 100;
    if (cali_data.adjust_curr > CHARGE_mA_MAX) {
        log_info("Set current out of adjustment range");
        ret = CALIBRATION_MA_ERR;
    } else {
        set_charge_mA(cali_data.adjust_curr);
    }
    return ret;
}

void charge_calibration_set_current_limit(u32 max_current, u32 min_current)
{
    ASSERT(max_current > min_current, "current limit set err, max: %d, min: %d\n", max_current, min_current);
    ASSERT((max_current -  min_current) >= 100, "current diff < 1mA set err, max: %d, min: %d\n", max_current, min_current);
    ASSERT(((max_current > config_charge_current) && (min_current < config_charge_current)), \
           "config_charge_current is out of range, max: %d, min: %d, config:%d\n", max_current, min_current, config_charge_current);
    calc_max_cur = max_current;
    calc_min_cur = min_current;
}

u8 charge_calibration_report_current(u32 current)
{
    u8 ret = CALIBRATION_SUCC;
    now_current = current;
    if (calc_max_cur == 0) {
        calc_max_cur = (TCFG_CHARGE_MA + MAX_CUR_OFFSET) * 100;
    }
    if (calc_min_cur == 0) {
        calc_min_cur = (TCFG_CHARGE_MA - MIN_CUR_OFFSET) * 100;
    }
    log_info("max: %d, min: %d, now: %d, config:%d\n", calc_max_cur, calc_min_cur, now_current, config_charge_current);
    if ((now_current > calc_max_cur) || (now_current < calc_min_cur)) {
        if (now_current > config_charge_current) {
            ret = charge_calibration_bigger(now_current - config_charge_current);
        } else {
            ret = charge_calibration_smaller(config_charge_current - now_current);
        }
    }
    return ret;
}

calibration_result charge_calibration_get_result(void)
{
    calibration_result res;
    res.cali_current = cali_data.adjust_curr;
    return res;
}


#endif
