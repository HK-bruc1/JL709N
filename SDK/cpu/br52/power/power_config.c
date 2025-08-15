#include "asm/power_interface.h"
#include "app_config.h"
#include "gpio_config.h"

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

    key_wakeup_init();

}
