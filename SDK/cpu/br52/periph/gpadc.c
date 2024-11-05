#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".gpadc.data.bss")
#pragma data_seg(".gpadc.data")
#pragma const_seg(".gpadc.text.const")
#pragma code_seg(".gpadc.text")
#endif
#include "typedef.h"
#include "clock.h"
#include "gpadc.h"
#include "timer.h"
#include "init.h"
#include "asm/efuse.h"
#include "irq.h"
/* #include "p33.h" */
/* #include "asm/power/p11.h" */
/* #include "asm/power_interface.h" */
#include "jiffies.h"
#include "app_config.h"
#include "gpio.h"
#include "clock.h"
#include "asm/power_interface.h"

//br52

static u8 adc_ch_io_table[16] = {  //gpio->adc_ch 表
    IO_PORTB_00,
    IO_PORTB_01,
    IO_PORTB_02,
    IO_PORTB_03,
    IO_PORTB_04,
    IO_PORTB_05,
    IO_PORTA_00,
    IO_PORTC_01,
    IO_PORTC_02,
    IO_PORTC_03,
    IO_PORTC_04,
    IO_PORTC_05,
    IO_PORTC_06,
    IO_PORTC_07,
    IO_PORT_DP,
    IO_PORT_DM,
};

static void clock_critical_enter(void)
{
}
static void adc_adjust_div(void)
{
    const u8 adc_div_table[] = {1, 6, 12, 24, 48, 72, 96, 128};
    const u32 lsb_clk = clk_get("lsb");
    adc_clk_div = 7;
    for (int i = 0; i < ARRAY_SIZE(adc_div_table); i++) {
        if (lsb_clk / adc_div_table[i] <= 1000000) {
            adc_clk_div = i;
            break;
        }
    }
}
CLOCK_CRITICAL_HANDLE_REG(saradc, clock_critical_enter, adc_adjust_div)


u32 adc_io2ch(int gpio)   //根据传入的GPIO，返回对应的ADC_CH
{
    for (u8 i = 0; i < ARRAY_SIZE(adc_ch_io_table); i++) {
        if (adc_ch_io_table[i] == gpio) {
            return (ADC_CH_TYPE_IO | i);
        }
    }
    printf("add_adc_ch io error!!! change other io_port!!!\n");
    return 0xffffffff; //未找到支持ADC的IO
}

void adc_io_ch_set(enum AD_CH ch, u32 mode) //adc io通道 模式设置
{

}

void adc_internal_signal_to_io(u32 analog_ch, u16 adc_io) //将内部通道信号，接到IO口上，输出
{
    gpio_set_mode(IO_PORT_SPILT(adc_io), PORT_HIGHZ);
    /* gpio_set_function(IO_PORT_SPILT(adc_io), PORT_FUNC_GPADC); */


    adc_sample(analog_ch, 0);
    u32 ch = adc_io2ch(adc_io);
    u16 adc_ch_sel = ch & ADC_CH_MASK_CH_SEL;
    SFR(JL_ADC->CON, 8, 4, adc_ch_sel);
    SFR(JL_ADC->CON, 21, 3, 0b111);
}

static void _adc_pmu_vbg_enable()
{
    SFR(P3_PMU_ADC0, 2, 1, 1);
    udelay(11);
    udelay(11);
    udelay(11);
    udelay(11);
    udelay(11);
    SFR(P3_PMU_ADC0, 3, 1, 1);
}
static void _adc_pmu_vbg_diable()
{
    SFR(P3_PMU_ADC0, 3, 1, 0);
    udelay(11);
    udelay(11);
    udelay(11);
    udelay(11);
    udelay(11);
    SFR(P3_PMU_ADC0, 2, 1, 0);
}
static void adc_pmu_ch_select(u32 ch)
{
    u8 vbg_ch = ((ch & ADC_CH_MASK_PMU_VBG_CH_SEL) >> 8);
    u8 pmu_ch = (ch & ADC_CH_MASK_CH_SEL);
    if (pmu_ch == 0) {  //VBG通道
        SFR(P3_PMU_ADC0, 4, 2, vbg_ch);
        SFR(P3_PMU_ADC0, 3, 1, 1);  //VBG_TEST_EN
    }
    SFR(P3_PMU_ADC1, 0, 5, pmu_ch);
    SFR(P3_PMU_ADC0, 1, 1, 1);  //PMU_TEST_OE ENABLE
    SFR(P3_PMU_ADC0, 0, 1, 1);  //PMU_TOADC_EN  ENABLE
}

static void adc_audio_ch_select(u32 ch_sel) //Audio通道切换
{
    SFR(JL_ADDA->ADDA_CON0,  0, 13,  0);
    SFR(JL_ADDA->ADDA_CON0,  0,  1,  1);					// AUDIO到SARADC的总测试通道使能

    switch (ch_sel) {
    case ADC_CH_AUDIO_IRP5U:
        SFR(JL_ADDA->ADDA_CON0, 1, 1, 1);
        break;
    case ADC_CH_AUDIO_VCM:
        SFR(JL_ADDA->ADDA_CON0, 2, 1, 1);
        break;
    default:
        break;
    }
    u32 ch = ch_sel & 0xffff;
    if (ch <= 0x16) {
        SFR(JL_ADDA->ADDA_CON0,  7,  1,  1);	// ADC测试通道总使能
        SFR(JL_ADDA->ADDA_CON0,  8,  5,  ch);	// ADC待测试信号选择
    } else if (ch <= 0x1e) {
        SFR(JL_ADDA->ADDA_CON0,  3,  1,  1);	// DAC测试通道总使能
        SFR(JL_ADDA->ADDA_CON0,  4,  3,  ch - 0x17);	// DAC待测试信号选择位
    }
}

u32 adc_add_sample_ch(enum AD_CH ch)   //添加一个指定的通道到采集队列
{
    u32 adc_type_sel = ch & ADC_CH_MASK_TYPE_SEL;
    u16 adc_ch_sel = ch & ADC_CH_MASK_CH_SEL;
    printf("type = %x,ch = %x\n", adc_type_sel, adc_ch_sel);
    u32 i = 0;
    for (i = 0; i < ADC_MAX_CH; i++) {
        if (adc_queue[i].ch == ch) {
            break;
        } else if (adc_queue[i].ch == -1) {
            adc_queue[i].ch = ch;
            adc_queue[i].v.value = 1;
            adc_queue[i].sample_period = 0;

            switch (ch) {
            case AD_CH_PMU_VBAT:
                adc_queue[i].adc_voltage_mode = 1;
                break;
            case AD_CH_PMU_VTEMP:
                adc_queue[i].adc_voltage_mode = 1;
                break;
            default:
                adc_queue[i].adc_voltage_mode = 0;
                break;
            }
            // if (adc_type_sel == ADC_CH_TYPE_IO) {
            //     printf("IO dma sample\n");
            //     JL_ADC->CON &= ~BIT(4);
            //     SFR(JL_ADC->DMA_CON0, 16, 16, (JL_ADC->DMA_CON0 >> 16) + 1);//DMA长度加1
            //     JL_ADC->DMA_CON1 |= BIT(adc_ch_sel);//使能对应DMA通道
            // } else {
            //     /* JL_ADC->CON |= BIT(1); */
            // }
            printf("add sample ch 0x%x, i = %d\n", ch, i);
            break;
        }
    }
    return i;
}

u32 adc_delete_ch(enum AD_CH ch)    //将一个指定的通道从采集队列中删除
{
    u32 i = 0;
    for (i = 0; i < ADC_MAX_CH; i++) {
        if (adc_queue[i].ch == ch) {
            adc_queue[i].ch = -1;
            break;
        }
    }
    return i;
}

void adc_sample(enum AD_CH ch, u32 ie) //启动一次cpu模式的adc采样
{
    u32 adc_con = 0;
    SFR(adc_con, 0, 3, adc_clk_div);//adc_clk 分频
    adc_con |= (0x1 << 12); //启动延时控制，实际启动延时为此数值*8个ADC时钟
    adc_con |= BIT(3) | BIT(4); //ana en
    adc_con |= BIT(6); //clr pend
    SFR(adc_con, 24, 2, 0b10);//isel
    if (ie) {
        adc_con |= BIT(5);//ie
    }
    adc_con |= BIT(17);//clk en

    u32 adc_type_sel = ch & ADC_CH_MASK_TYPE_SEL;
    u16 adc_ch_sel = ch & ADC_CH_MASK_CH_SEL;
    SFR(adc_con, 21, 3, 0b010);//cpu adc test sel en
    SFR(adc_con, 18, 3, adc_type_sel >> 16);    //test sel
    switch (adc_type_sel) {
    case ADC_CH_TYPE_BT:
        break;
    case ADC_CH_TYPE_PMU:
        /* adc_pmu_ch_select(adc_ch_sel); */
        adc_pmu_ch_select(ch);
        break;
    case ADC_CH_TYPE_AUDIO:
        adc_audio_ch_select(ch);
        break;
    case ADC_CH_TYPE_LPCTMU:
        break;
    case ADC_CH_TYPE_SYSPLL:
        break;
    default:
        SFR(adc_con, 21, 3, 0b001); //cpu adc io sel en
        SFR(adc_con, 8, 4, adc_ch_sel);
        break;
    }
    JL_ADC->CON = adc_con;
    //if (ch == AD_CH_LDOREF) {
    //    JL_PORTUSB->DIR &= ~BIT(1);
    //    JL_PORTUSB->OUT |= BIT(1);
    //    printf("ADC_CON %x, %x, P3_PMU_ADC0 %x", JL_ADC->CON, adc_con, P3_PMU_ADC0);
    //}
    JL_ADC->CON |= BIT(6);//kistart
}
static void adc_wait_idle_timeout()
{
    u32 time_out_us = 1000;
    while (time_out_us--) {
        asm("csync");
        if (JL_ADC->CON & BIT(7)) {
            break;
        }
        if ((JL_ADC->CON & BIT(4)) == 0) {
            break;
        }
        udelay(1);
    }
}
void adc_wait_enter_idle() //等待adc进入空闲状态，才可进行阻塞式采集
{
    if (JL_ADC->CON & BIT(4)) {
        adc_wait_idle_timeout();
        adc_close();
    } else {
        return ;
    }
}
void adc_set_enter_idle() //设置 adc cpu模式为空闲状态
{
    /* JL_ADC->CON &= ~BIT(4);//en */
}
u16 adc_wait_pnd()   //cpu采集等待pnd
{
    adc_wait_idle_timeout();
    u32 adc_res = JL_ADC->RES;
    adc_close();
    return adc_res;
}

void adc_dma_sample(enum AD_CH ch, u16 *buf, u32 sample_times)
{
    /* JL_PORTA->DIR &= ~BIT(7); */
    /* JL_PORTA->OUT |= BIT(7); */

    adc_queue[ADC_MAX_CH].ch = ch;

    adc_wait_enter_idle();

    local_irq_disable();

    adc_sample(ch, 0);

    for (u32 i = 0; i < sample_times; i++) {
        adc_wait_idle_timeout();
        buf[i] = JL_ADC->RES;
        /* printf("res %d\n", buf[i]); */
        JL_ADC->CON |= BIT(6);
    }
    adc_close();

    adc_queue[ADC_MAX_CH].ch = -1;

    adc_set_enter_idle();

    local_irq_enable();

    /* JL_PORTA->OUT &= ~BIT(7); */
}

void adc_close()     //adc close
{
    JL_ADC->CON = BIT(17);//clock_en
    JL_ADC->CON = BIT(17) | BIT(6);
    JL_ADC->CON = BIT(6);
}

___interrupt
static void adc_isr()   //中断函数
{
    //JL_PORTUSB->OUT &= ~BIT(1);
    if (adc_queue[ADC_MAX_CH].ch != -1) {
        adc_close();
        return;
    }

    if (JL_ADC->CON & BIT(7)) {
        u32 adc_value = JL_ADC->RES;

        if (adc_cpu_mode_process(adc_value)) {
            return;
        }
        cur_ch++;
        cur_ch = adc_get_next_ch();
        if (cur_ch == ADC_MAX_CH) {
            adc_close();
        } else {
            adc_sample(adc_queue[cur_ch].ch, 1);
        }
    }
}

static void adc_scan()  //定时函数，每 x ms启动一轮cpu模式采集
{
    if (adc_queue[ADC_MAX_CH].ch != -1) {
        return;
    }
    if (JL_ADC->CON & BIT(4)) {
        return;
    }
    cur_ch = adc_get_next_ch();
    if (cur_ch == ADC_MAX_CH) {
        return;
    }
    adc_sample(adc_queue[cur_ch].ch, 1);
}

#include "gptimer.h"
void adc_hw_init(void)    //adc初始化子函数
{
    memset(adc_queue, 0xff, sizeof(adc_queue));

    _adc_pmu_vbg_enable();

    adc_close();

    adc_adjust_div();

    adc_add_sample_ch(AD_CH_LDOREF);
    adc_set_sample_period(AD_CH_LDOREF, PMU_CH_SAMPLE_PERIOD);

    adc_add_sample_ch(AD_CH_PMU_VBAT);
    adc_set_sample_period(AD_CH_PMU_VBAT, PMU_CH_SAMPLE_PERIOD);

    adc_add_sample_ch(AD_CH_PMU_VTEMP);
    adc_set_sample_period(AD_CH_PMU_VTEMP, PMU_CH_SAMPLE_PERIOD);

    u32 vbg_adc_value = 0;
    u32 vbg_min_value = -1;
    u32 vbg_max_value = 0;

    for (int i = 0; i < AD_CH_PMU_VBG_TRIM_NUM; i++) {
        u32 adc_value = adc_get_value_blocking(AD_CH_LDOREF);
        if (adc_value > vbg_max_value) {
            vbg_max_value = adc_value;
        } else if (adc_value < vbg_min_value) {
            vbg_min_value = adc_value;
        }
        vbg_adc_value += adc_value;
    }
    vbg_adc_value -= vbg_max_value;
    vbg_adc_value -= vbg_min_value;

    vbg_adc_value /= (AD_CH_PMU_VBG_TRIM_NUM - 2);
    adc_queue[0].v.value = vbg_adc_value;
    printf("LDOREF = %d\n", vbg_adc_value);

    u32 voltage = adc_get_voltage_blocking(AD_CH_PMU_VBAT);
    adc_queue[1].v.voltage = voltage;
    printf("vbat = %d mv\n", adc_get_voltage(AD_CH_PMU_VBAT) * 4);

    voltage = adc_get_voltage_blocking(AD_CH_PMU_VTEMP);
    adc_queue[2].v.voltage = voltage;
    printf("dtemp = %d mv\n", voltage);

    /* read_hpvdd_voltage(); */
    //切换到vbg通道

    request_irq(IRQ_GPADC_IDX, 1, adc_isr, 0);
    usr_timer_add(NULL, adc_scan,  5, 0);

    /* void adc_test_demo(); */
    /* usr_timer_add(NULL, adc_test_demo, 1000, 0); //打印信息 */
    /* adc_internal_signal_to_io(AD_CH_LDOREF, IO_PORTC_01); //将内部通道信号，接到IO口上，输出 */
    /* while (1) { */
    /*     extern void wdt_clear(); */
    /*     wdt_clear(); */
    /* } */
}

static void adc_test_demo()  //adc测试函数，根据需求搭建
{
    /* printf("\n\n%s() CHIP_ID :%x\n", __func__, get_chip_version()); */
    printf("%s() VBG:%d\n", __func__, adc_get_value(AD_CH_LDOREF));
    printf("%s() VBAT:%d mv\n", __func__, adc_get_voltage(AD_CH_PMU_VBAT) * 4);
    /* printf("%s() DTEMP:%d\n", __func__, adc_get_voltage(AD_CH_PMU_VTEMP)); */
    /* printf("%s() PB4:%d\n", __func__, adc_get_value(adc_io2ch(IO_PORTB_04))); */
    printf("%s() PC1:%d\n", __func__, adc_get_value_blocking(adc_io2ch(IO_PORTC_01)));
    printf("%s() PC1:%dmv\n", __func__, adc_get_voltage_blocking(adc_io2ch(IO_PORTC_01)));
}

static u8 gpadc_idle_query(void)
{
    if (JL_ADC->CON & BIT(4)) {
        return 0; //不可以进入休眠
    }
    return 1; //可以进入休眠
}
REGISTER_LP_TARGET(gpadc_driver_target) = {
    .name = "gpadc",
    .is_idle = gpadc_idle_query,
};
