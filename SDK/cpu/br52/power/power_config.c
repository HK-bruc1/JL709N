#include "asm/power_interface.h"
#include "app_config.h"
#include "gpio_config.h"
#include "in_ear_detect/in_ear_manage.h"
//-----------------------------------------------------------------------------------------------------------------------
/* config
 */
#define POWER_PARAM_CONFIG				TCFG_LOWPOWER_LOWPOWER_SEL
#define POWER_PARAM_BTOSC_HZ			TCFG_CLOCK_OSC_HZ
#define POWER_PARAM_VDDIOM_LEV			TCFG_LOWPOWER_VDDIOM_LEVEL
#define POWER_PARAM_VDDIOW_LEV			TCFG_LOWPOWER_VDDIOW_LEVEL
#define POWER_PARAM_OSC_TYPE			TCFG_LOWPOWER_OSC_TYPE

#define CONFIG_POWER_MODE				TCFG_LOWPOWER_POWER_SEL
#define CONFIG_DCDC_TYPE				TCFG_DCDC_TYPE
#define CONFIG_CHARGE_ENABLE			TCFG_CHARGE_ENABLE

const u32 dvdd_cap_en = TCFG_DVDD_CAP_EN;

//-----------------------------------------------------------------------------------------------------------------------
/* power_param
 */
struct _power_param power_param = {
    .config         = POWER_PARAM_CONFIG,
    .btosc_hz       = POWER_PARAM_BTOSC_HZ,
    .vddiom_lev     = POWER_PARAM_VDDIOM_LEV,
    .vddiow_lev     = POWER_PARAM_VDDIOW_LEV,
    .osc_type       = POWER_PARAM_OSC_TYPE,
};

//----------------------------------------------------------------------------------------------------------------------
/* power_pdata
 */
struct _power_pdata power_pdata = {
    .power_param_p  = &power_param,
};

//----------------------------------------------------------------------------------------------------------------------

#if TCFG_EAR_DETECT_ENABLE
#if (TCFG_EAR_DETECT_TYPE == EAR_DETECT_BY_TOUCH) && (!TCFG_EAR_DETECT_TOUCH_MODE)
struct _p33_io_wakeup_config port2 = {
	.pullup_down_mode = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = !TCFG_EAR_DETECT_DET_LEVEL,                            //唤醒方式选择,可选：上升沿\下降沿
    .filter             = PORT_FLT_16ms,
	.gpio              = TCFG_EAR_DETECT_DET_IO,                             //唤醒口选择
};
#endif
#endif


#if TCFG_EAR_DETECT_ENABLE
static const struct wakeup_param wk_param = {
    #if (TCFG_EAR_DETECT_TYPE == EAR_DETECT_BY_TOUCH) && (!TCFG_EAR_DETECT_TOUCH_MODE)
	.port[0] = &port2,
#endif
};
#endif

#if TCFG_EAR_DETECT_ENABLE
static void port_wakeup_callback(u8 index, u8 gpio)
{
    switch (index) {
#if (TCFG_TEST_BOX_ENABLE || TCFG_CHARGESTORE_ENABLE || TCFG_ANC_BOX_ENABLE)
    case 2:
        extern void chargestore_ldo5v_fall_deal(void);
        chargestore_ldo5v_fall_deal();
        break;
#endif
#if TCFG_EAR_DETECT_ENABLE
#if ((TCFG_EAR_DETECT_TYPE == EAR_DETECT_BY_TOUCH) && (!TCFG_EAR_DETECT_TOUCH_MODE))
	if (gpio == TCFG_EAR_DETECT_DET_IO) {
		ear_touch_edge_wakeup_handle(index, gpio);
	}
#endif
#endif /* #if TCFG_EAR_TCH_ENABLE */

    }
}
#endif

void key_wakeup_init();

void board_power_init()
{
    gpio_config_init();

    power_control(PCONTROL_PD_VDDIO_KEEP, VDDIO_KEEP_TYPE_NORMAL);
    power_control(PCONTROL_SF_VDDIO_KEEP, VDDIO_KEEP_TYPE_NORMAL);

    power_control(PCONTROL_SF_KEEP_NVDD, 1);
    power_set_dcdc_type(CONFIG_DCDC_TYPE);

    power_init(&power_pdata);

#if (!CONFIG_CHARGE_ENABLE)
    power_set_mode(CONFIG_POWER_MODE);
    //没开启充电时,关闭漏电寄存器(约2uA)
    CHG_VILOOP_EN(0);
    CHG_VILOOP2_EN(0);
#endif

#if TCFG_EAR_DETECT_ENABLE
    ear_detect_init();
#endif

    key_wakeup_init();

}
