#include "app_config.h"
#include "system/includes.h"
#include "asm/power_interface.h"
#include "poweroff.h"


#if TCFG_LOWPOWER_POWER_SEL == PWR_DCDC15


#define BUCK2_PWM_CLK_BUF_SIZE  3
static u32 buck2_pwm_clk_buf[BUCK2_PWM_CLK_BUF_SIZE];
static u8 buck2_pwm_clk_idx;


u32 get_buck2_pwm_clk(void)
{
    u32 clk = 0;
    u32 i, j = 0;
    for (i = 0; i < BUCK2_PWM_CLK_BUF_SIZE; i ++) {
        if (buck2_pwm_clk_buf[i]) {
            clk += buck2_pwm_clk_buf[i];
            j ++;
        }
    }
    if (j) {
        clk /= j;
    }
    return clk;
}

void check_dcvdd_ocp(void)
{
    u32 dcdc_osc_clk = 2000000;
    u32 dcvdd_ocp_clk = dcdc_osc_clk * 99 / 100;
    u32 buck2_pwm_clk = get_buck2_pwm_clk();

    /* printf("buck2_pwm_clk = %dHz\n", buck2_pwm_clk); */
    /* printf("dcvdd_ocp_clk = %dHz\n", dcvdd_ocp_clk); */

    if (buck2_pwm_clk > dcvdd_ocp_clk) {//dcvdd 电源域过流
        /* post event --> dcvdd cop,  @pm */
        printf("check_dcvdd_ocp poweroff!\n");
        int argv[3];
        argv[0] = (int)sys_enter_soft_poweroff;
        argv[1] = 1;
        argv[2] = POWEROFF_NORMAL;
        int ret = os_taskq_post_type("app_core", Q_CALLBACK, 3, argv);
        if (ret) {
            printf("check_dcvdd_ocp_in_irq taskq post err\n");
        }
    }
}

___interrupt
void gpcnt_isr(void)
{
    JL_GPCNT0->CON |= BIT(30);

    u64 num = JL_GPCNT0->NUM;
    num *= 24000000L;
    num /= 1048576;
    buck2_pwm_clk_buf[buck2_pwm_clk_idx] = (u32)num;
    buck2_pwm_clk_idx ++;
    buck2_pwm_clk_idx %= BUCK2_PWM_CLK_BUF_SIZE;

    check_dcvdd_ocp();

    JL_GPCNT0->CON &= ~BIT(0);
    JL_GPCNT0->CON |= BIT(30) | BIT(0);
}

void gpcnt_buck2_pwm_clk_init(void *priv)
{
    buck2_pwm_clk_idx = 0;
    for (u32 i = 0; i < BUCK2_PWM_CLK_BUF_SIZE; i ++) {
        buck2_pwm_clk_buf[i] = 0;
    }

    P33_CON_SET(P3_ANA_MFIX, 0, 1, 1);  //buck2_pwm_clk test oe
    P33_CON_SET(P3_DBG_CON0, 0, 3, 7);  //p33_dbg_out sel buck2_pwm_clk

    JL_GPCNT0->CON = BIT(30);

    request_irq(IRQ_GPCNT0_IDX, 1, gpcnt_isr, 0);

    SFR(JL_GPCNT0->CON,  1,  5, 10); //次时钟选择p33_dbg_out

    SFR(JL_GPCNT0->CON, 16,  4, 15); //主时钟周期数：32 * 2^n
    SFR(JL_GPCNT0->CON,  8,  5, 15); //主时钟选择std24m,  43ms

    JL_GPCNT0->CON |= BIT(30) | BIT(0);

    printf("P3_ANA_MFIX = 0x%x\n", P3_ANA_MFIX);
    printf("P3_DBG_CON0 = 0x%x\n", P3_DBG_CON0);
    printf("JL_GPCNT0->CON = 0x%x\n", JL_GPCNT0->CON);
}

static int dcvdd_ocp_init(void)
{
    sys_timeout_add(NULL, gpcnt_buck2_pwm_clk_init, 100);//late_initcall之后100ms开始执行，避开触摸的gpcnt
    return 0;
}
late_initcall(dcvdd_ocp_init);


static u32 gpcnt_con;

static u8 gpcnt_enter_sleep(void)
{
    JL_GPCNT0->CON &= ~BIT(0);
    gpcnt_con = JL_GPCNT0->CON;
    return 0;
}

static u8 gpcnt_exit_sleep(void)
{
    JL_GPCNT0->CON = gpcnt_con;
    JL_GPCNT0->CON |= BIT(30) | BIT(0);
    return 0;
}

SLEEP_TARGET_REGISTER(sleep_gpcnt) = {
    .name   = "gpcnt",
    .enter  = gpcnt_enter_sleep,
    .exit   = gpcnt_exit_sleep,
};

#endif

