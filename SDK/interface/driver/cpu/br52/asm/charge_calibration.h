#ifndef _CHARGE_CALIBRATION_H_
#define _CHARGE_CALIBRATION_H_

#include "typedef.h"

//电流校准的返回值
#define CALIBRATION_SUCC            0   //正常
#define CALIBRATION_MA_ERR          1   //设置的电流超出调整范围
#define CALIBRATION_CONTINUE        0xFF

typedef struct _calibration_result_ {
    u16 cali_current;
} calibration_result;

calibration_result charge_calibration_get_result(void);
void charge_calibration_set_current_limit(u32 max_current, u32 min_current);
u8 charge_calibration_report_current(u32 current);
void charge_enter_calibration_mode(void);
#endif    //_CHARGE_CALIBRATION_H_

