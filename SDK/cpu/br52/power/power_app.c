#include "asm/power_interface.h"
#include "init.h"
#include "app_config.h"
#include "cpu/includes.h"
#include "gpio_config.h"

//-------------------------------------------------------------------
/*config
 */
#define CONFIG_UART_DEBUG_ENABLE	CONFIG_DEBUG_ENABLE
#ifdef TCFG_DEBUG_UART_TX_PIN
#define CONFIG_UART_DEBUG_PORT		TCFG_DEBUG_UART_TX_PIN
#else
#define CONFIG_UART_DEBUG_PORT		-1
#endif

#define DO_PLATFORM_UNINITCALL()	do_platform_uninitcall()


/*-----------------------------------------------------------------------
 *进入、退出低功耗函数回调状态，函数单核操作、关中断，请勿做耗时操作
 *
 */
static u32 usb_io_con[5] = {0};
void sleep_enter_callback(u8 step)
{
    /* 此函数禁止添加打印 */
    u32 value = 0xffff;
    putchar('<');

    //USB IO打印引脚特殊处理
#if (CONFIG_UART_DEBUG_ENABLE && ((CONFIG_UART_DEBUG_PORT == IO_PORT_DP) || (CONFIG_UART_DEBUG_PORT == IO_PORT_DM))) || \
    (TCFG_CFG_TOOL_ENABLE && (TCFG_COMM_TYPE == TCFG_UART_COMM) && ((TCFG_ONLINE_TX_PORT == IO_PORT_DP) || (TCFG_ONLINE_RX_PORT == IO_PORT_DM))) || \
    (TCFG_CFG_TOOL_ENABLE && (TCFG_COMM_TYPE == TCFG_USB_COMM)) || \
    TCFG_USB_HOST_ENABLE || TCFG_PC_ENABLE
    usb_io_con[0] = JL_PORTUSB->DIR;
    usb_io_con[1] = JL_PORTUSB->DIE;
    usb_io_con[2] = JL_PORTUSB->DIEH;
    usb_io_con[3] = JL_PORTUSB->PU0;
    usb_io_con[4] = JL_PORTUSB->PD0;
#if TCFG_CHARGESTORE_PORT == IO_PORT_DP
    //fix: 串口升级配了DP脚下拉唤醒，进低功耗的时候把DP拉低了又唤醒了，就一直没怎么进低功耗
    value &= ~BIT(0);
#endif
#endif

    gpio_close(PORTUSB, value);
}

void sleep_exit_callback(u32 usec)
{
    //USB IO打印引脚特殊处理
#if (CONFIG_UART_DEBUG_ENABLE && ((CONFIG_UART_DEBUG_PORT == IO_PORT_DP) || (CONFIG_UART_DEBUG_PORT == IO_PORT_DM))) || \
    (TCFG_CFG_TOOL_ENABLE && (TCFG_COMM_TYPE == TCFG_UART_COMM) && ((TCFG_ONLINE_TX_PORT == IO_PORT_DP) || (TCFG_ONLINE_RX_PORT == IO_PORT_DM))) || \
    (TCFG_CFG_TOOL_ENABLE && (TCFG_COMM_TYPE == TCFG_USB_COMM)) || \
    TCFG_USB_HOST_ENABLE || TCFG_PC_ENABLE
    JL_PORTUSB->PD0  = usb_io_con[4];
    JL_PORTUSB->PU0  = usb_io_con[3];
    JL_PORTUSB->DIEH = usb_io_con[2];
    JL_PORTUSB->DIE  = usb_io_con[1];
    JL_PORTUSB->DIR  = usb_io_con[0];
#endif

    putchar('>');

}

//maskrom 使用到的io
static void __mask_io_cfg()
{
    struct app_soft_flag_t app_soft_flag = {0};

    mask_softflag_config(&app_soft_flag);
}

u8 power_soff_callback()
{
    DO_PLATFORM_UNINITCALL();

    __mask_io_cfg();

    void gpio_config_soft_poweroff(void);
    gpio_config_soft_poweroff();

    gpio_config_uninit();

    return 0;
}

void power_early_flowing()
{
    PORT_TABLE(g);

    init_boot_rom();

    printf("get_boot_rom(): %d", get_boot_rom());

    // 默认关闭长按复位0，由key_driver配置
    gpio_longpress_pin0_reset_config(IO_PORTA_03, 0, 0, 1, 1);
    //长按复位1默认配置8s，写保护
    //gpio_longpress_pin1_reset_config(IO_LDOIN_DET, 0, 0, 1);//临时关闭 by IC :luochangen

#if CONFIG_UART_DEBUG_ENABLE
    PORT_PROTECT(CONFIG_UART_DEBUG_PORT);
#endif

    power_early_init((u32)gpio_config);
}

//early_initcall(_power_early_init);

static int power_later_flowing()
{
    pmu_trim(0, 0);

    power_later_init(0);

    return 0;
}

late_initcall(power_later_flowing);

