#include "asm/includes.h"
#include "system/includes.h"
#include "app_config.h"

#define LOG_TAG             "[SETUP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

extern void sys_timer_init(void);
extern void tick_timer_init(void);
extern void exception_irq_handler(void);
extern int __crc16_mutex_init();
extern void debug_uart_init();
extern void boot_power_init();

/* --------------------------------------------------------------------------*/
/**
 * @brief 裸机程序入口
 */
/* ----------------------------------------------------------------------------*/
void system_main(void)
{
    //分支预测
    q32DSP(core_num())->PMU_CON1 &= ~BIT(8);

    wdt_close();

#if (TCFG_MAX_LIMIT_SYS_CLOCK==MAX_LIMIT_SYS_CLOCK_160M)
    clk_early_init(PLL_REF_XOSC_DIFF, TCFG_CLOCK_OSC_HZ, 240 * MHz);//  240:max clock 160
#else
    clk_early_init(PLL_REF_XOSC_DIFF, TCFG_CLOCK_OSC_HZ, 192 * MHz);
#endif

    //heap内存管理，STDIO需要heap
    memory_init();

    //uart init
    debug_uart_init();

    //STDIO init
    log_early_init(1024 * 2);

    //debug info

    while (1) {
        asm("idle");
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 裸机环境启动，进入无操作系统环境
 */
/* ----------------------------------------------------------------------------*/
#define SYSTEM_SSP_SIZE 512
u32 system_ssp_stack[SYSTEM_SSP_SIZE];//2Kbyte
void system_start(void)
{
    asm("ssp = %0"::"i"(system_ssp_stack + SYSTEM_SSP_SIZE));
    asm("r0 = %0"::"i"(system_main));
    asm("reti = r0");
    asm("rti");
}


#if CONFIG_DEBUG_ENABLE || CONFIG_DEBUG_LITE_ENABLE
#endif

#if 0
___interrupt
void exception_irq_handler(void)
{
    ___trig;

    exception_analyze();

    log_flush();
    while (1);
}
#endif



/*
 * 此函数在cpu0上电后首先被调用,负责初始化cpu内部模块
 *
 * 此函数返回后，操作系统才开始初始化并运行
 *
 */

#if 0
static void early_putchar(char a)
{
    if (a == '\n') {
        UT2_BUF = '\r';
        __asm_csync();
        while ((UT2_CON & BIT(15)) == 0);
    }
    UT2_BUF = a;
    __asm_csync();
    while ((UT2_CON & BIT(15)) == 0);
}

void early_puts(char *s)
{
    do {
        early_putchar(*s);
    } while (*(++s));
}
#endif

void cpu_assert_debug()
{
#if CONFIG_DEBUG_ENABLE
    log_flush();
    local_irq_disable();
    while (1);
#else
    system_reset(ASSERT_FLAG);
#endif
}

_NOINLINE_
void cpu_assert(char *file, int line, bool condition, char *cond_str)
{
    if (config_asser) {
        if (!(condition)) {
            printf("cpu %d file:%s, line:%d\n", current_cpu_id(), file, line);
            printf("ASSERT-FAILD: %s\n", cond_str);
            cpu_assert_debug();
        }
    } else {
        if (!(condition)) {
            system_reset(ASSERT_FLAG);
        }
    }
}

extern void sputchar(char c);
extern void sput_buf(const u8 *buf, int len);
void sput_u32hex(u32 dat);



__attribute__((weak))
void maskrom_init(void)
{
    return;
}


#if (CPU_CORE_NUM > 1)
void cpu1_setup_arch()
{
    q32DSP(core_num())->PMU_CON1 &= ~BIT(8); //open bpu

    request_irq(IRQ_EXCEPTION_IDX, 7, exception_irq_handler, 1);

    //用于控制其他核进入停止状态。
    extern void cpu_suspend_handle(void);
    request_irq(IRQ_SOFT0_IDX, 7, cpu_suspend_handle, 0);
    request_irq(IRQ_SOFT1_IDX, 7, cpu_suspend_handle, 1);
    irq_unmask_set(IRQ_SOFT0_IDX, 0, 0); //设置CPU0软中断0为不可屏蔽中断
    irq_unmask_set(IRQ_SOFT1_IDX, 0, 1); //设置CPU1软中断1为不可屏蔽中断

    debug_init();
}

void cpu1_main()
{
    extern void cpu1_run_notify(void);
    cpu1_run_notify();

    interrupt_init();

    q32DSP(core_num())->PMU_CON1 &= ~BIT(8); //分支预测
    cpu1_setup_arch();

    os_start();

    log_e("os err \r\n") ;
    while (1) {
        __asm__ volatile("idle");
    }
}

#else

void cpu1_main()
{

}
#endif /* #if (CPU_CORE_NUM > 1) */

//==================================================//

/* extern void gpio_longpress_pin0_reset_config(u32 pin, u32 level, u32 time); */
void memory_init(void);

__attribute__((weak))
void app_main()
{
    while (1) {
        asm("idle");
    }
}
#define     IIO_DEBUG_TOGGLE(i,x)  {JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT ^= BIT(x);}

void io_toggle()
{
    IIO_DEBUG_TOGGLE(A, 3);
    IIO_DEBUG_TOGGLE(A, 3);

}

int JL_AES_BASE_BT = (int)JL_AES;	// add for btcon_hash, by lingxuanfeng, 20220517

void setup_arch()
{
    q32DSP(core_num())->PMU_CON1 &= ~BIT(8); //分支预测

    memory_init();

    wdt_init(WDT_16S);
    /* wdt_close(); */

    gpadc_mem_init(8);
    efuse_init();

#if TCFG_AUDIO_DAC_VOLUME_BOOST
#include "asm/clock_hal.h"
    clk_vdc_mode_init(CLOCK_MODE_USR, DCVDD_VOL_1250V);
#endif

    clk_early_init(PLL_REF_XOSC_DIFF, TCFG_CLOCK_OSC_HZ, 192 * MHz);

    //临时添加：by IC wangjie
    SFR(JL_LSBCLK->PRP_CON1,  0, 3, 1);
    SFR(JL_LSBCLK->PRP_CON1, 26, 1, 1);

    // bt clk config by IC luokaijie
    SFR(JL_LSBCLK->PRP_CON2, 0, 2, 1);
    SFR(JL_LSBCLK->PRP_CON2, 2, 2, 1);
    SFR(JL_LSBCLK->PRP_CON2, 4, 2, 1);
    SFR(JL_LSBCLK->PRP_CON2, 8, 1, 1);
    asm("csync");

    os_init();
    tick_timer_init();
    sys_timer_init();

#if CONFIG_DEBUG_ENABLE || CONFIG_DEBUG_LITE_ENABLE
    debug_uart_init();
#if CONFIG_DEBUG_ENABLE
    log_early_init(1024 * 2);
#endif

#endif
    log_i("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    log_i("         setup_arch");
    log_i("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    power_early_flowing();

    clock_dump();

    extern void ic_vbg_current_trim();
    ic_vbg_current_trim();


    //Register debugger interrupt
    request_irq(0, 2, exception_irq_handler, 0);
    request_irq(1, 2, exception_irq_handler, 0);
    code_movable_init();
    debug_init();

    __crc16_mutex_init();

    ASSERT((u32)local_irq_disable < 0x120000);
    app_main();
}


