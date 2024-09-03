#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lpctmu_hw.data.bss")
#pragma data_seg(".lpctmu_hw.data")
#pragma const_seg(".lpctmu_hw.text.const")
#pragma code_seg(".lpctmu_hw.text")
#endif
#include "system/includes.h"
#include "asm/power_interface.h"
#include "asm/lpctmu_hw.h"
#include "gpadc.h"

#if 1

#define LOG_TAG_CONST       LP_KEY
#define LOG_TAG             "[LP_KEY]"
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"
#else

#define log_debug   printf

#endif



static const u8 ch_port[LPCTMU_CHANNEL_SIZE] = {
    IO_PORTB_00,
    IO_PORTB_01,
    IO_PORTB_02,
    IO_PORTB_03,
    IO_PORTB_04,
};

static struct lpctmu_config_data *__this = NULL;

extern u32 __get_lrc_hz();



void lpctmu_send_m2p_cmd(enum CTMU_M2P_CMD cmd)
{
    P2M_CTMU_CMD_ACK = 0;
    M2P_CTMU_CMD = cmd;
    P11_SYSTEM->M2P_INT_SET = BIT(M2P_CTMU_INDEX);
    while (!(P2M_CTMU_CMD_ACK));
}


u32 lpctmu_get_cur_ch_by_idx(u32 ch_idx)
{
    ch_idx %= __this->ch_num;
    return __this->ch_list[ch_idx];
}

u32 lpctmu_get_idx_by_cur_ch(u32 cur_ch)
{
    for (u32 ch_idx = 0; ch_idx < __this->ch_num; ch_idx ++) {
        if (cur_ch == __this->ch_list[ch_idx]) {
            return ch_idx;
        }
    }
    return 0;
}

void lpctmu_port_init(u32 ch)
{
    u8 port = ch_port[ch];
    gpio_set_mode(port / 16, BIT(port % 16), PORT_HIGHZ);
    SFR(P11_PORT->PB_SEL, ch, 1, 1);
    SFR(P11_PORT->PB_DIR, ch, 1, 1);
    SFR(P11_PORT->PB_DIE, ch, 1, 0);
    SFR(P11_PORT->PB_PU0, ch, 1, 0);
    SFR(P11_PORT->PB_PU1, ch, 1, 0);
    SFR(P11_PORT->PB_PD0, ch, 1, 0);
    SFR(P11_PORT->PB_PD1, ch, 1, 0);
}


void lpctmu_set_ana_hv_level(u32 level)
{
    SFR(P11_LPCTM0->ANA0, 5, 3, level & 0b111);
}

u32 lpctmu_get_ana_hv_level(void)
{
    u8 level = (P11_LPCTM0->ANA0 >> 5) & 0b111;
    return level;
}

void lpctmu_vsel_trim(void)
{
    if (__this->pdata->aim_vol_delta < 8) {
        lpctmu_set_ana_hv_level(__this->pdata->aim_vol_delta);
        SFR(P11_LPCTM0->ANA1, 0, 2, 0b00);//关闭HV/LV
        log_debug("hv_level = %d", __this->pdata->aim_vol_delta);
        return;
    }
    SFR(P11_LPCTM0->ANA1, 0, 2, 0b01);//先使能LV
    u32 lv_vol = adc_get_voltage_blocking(AD_CH_LPCTMU);  //阻塞式采集一个指定通道的电压值（经过均值滤波处理）
    log_debug("lv_vol = %dmv", lv_vol);

    SFR(P11_LPCTM0->ANA1, 0, 2, 0b10);//再使能HV
    u32 delta, diff, diff_min = -1;
    u8 aim_hv_level = 7;
    for (u8 hv_level = 0; hv_level < 8; hv_level ++) {
        lpctmu_set_ana_hv_level(hv_level);
        u32 hv_vol = adc_get_voltage_blocking(AD_CH_LPCTMU);  //阻塞式采集一个指定通道的电压值（经过均值滤波处理）
        log_debug("hv_vol = %dmv", hv_vol);
        if (hv_vol > lv_vol) {
            delta = hv_vol - lv_vol;
        } else {
            delta = lv_vol - hv_vol;
        }
        if (delta > __this->pdata->aim_vol_delta) {
            diff = delta - __this->pdata->aim_vol_delta;
        } else {
            diff = __this->pdata->aim_vol_delta - delta;
        }
        if (diff_min >= diff) {
            diff_min = diff;
            aim_hv_level = hv_level;
        }
    }
    log_debug("hv_level = %d  diff %d", aim_hv_level, diff_min);
    lpctmu_set_ana_hv_level(aim_hv_level);
    SFR(P11_LPCTM0->ANA1, 0, 2, 0b00);//关闭HV/LV
}


u32 lpctmu_get_charge_clk(void)
{
    JL_GPCNT0->CON = BIT(30);

    SFR(P11_SYSTEM->P11_SYS_CON0, 2, 1, 0);//msys tx dis
    SFR(P11_PORT->OCH_CON0, 8, 4, 9);//p11 output_ch2 sel lpctmu_test

    SFR(JL_GPCNT0->CON,  1,  5,  17);//次时钟选择p11_dbg_out

    SFR(JL_GPCNT0->CON, 16,  4,  8); //主时钟周期数：32 * 2^n = 8192
    SFR(JL_GPCNT0->CON,  8,  5, 15); //主时钟选择std24m

    JL_GPCNT0->CON |= BIT(30) | BIT(0);

    while (!(JL_GPCNT0->CON & BIT(31)));
    u32 gpcnt_num = JL_GPCNT0->NUM;
    JL_GPCNT0->CON &= ~BIT(0);

    u32 std24m_clk_khz = 24000;
    u32 charge_clk = std24m_clk_khz * gpcnt_num / 8192;
    log_debug("gpcnt_num = %d charge_clk = %dKHz\n", gpcnt_num, charge_clk);
    return charge_clk;
}

void lpctmu_set_ana_cur_level(u32 ch, u32 cur_level)
{
    SFR(P11_LPCTM0->CHIS, ch * 4, 3, cur_level);
}

u32 lpctmu_get_ana_cur_level(u32 ch)
{
    u32 level = (P11_LPCTM0->CHIS >> (ch * 4)) & 0b111;
    return level;
}

void lpctmu_isel_trim(u8 ch)
{
    SFR(P11_LPCTM0->ANA1, 4, 4, ch + 1);//使能对应通道

    u32 diff, diff_min = -1;
    u8 aim_cur_level;
    if (__this->pdata->aim_charge_khz < 8) {
        aim_cur_level = __this->pdata->aim_charge_khz;
    } else {
        for (u8 cur_level = 0; cur_level < 8; cur_level ++) {
            SFR(P11_LPCTM0->ANA0, 1, 3, cur_level);
            lpctmu_set_ana_cur_level(ch, cur_level);
            u32 charge_clk = lpctmu_get_charge_clk();
            if (charge_clk > __this->pdata->aim_charge_khz) {
                diff = charge_clk - __this->pdata->aim_charge_khz;
            } else {
                diff = __this->pdata->aim_charge_khz - charge_clk;
            }
            if (diff_min >= diff) {
                diff_min = diff;
                aim_cur_level = cur_level;
            }
        }
    }
    log_debug("ch%d cur_level = %d  diff %d", ch, aim_cur_level, diff_min);
    SFR(P11_LPCTM0->ANA0, 1, 3, aim_cur_level);
    lpctmu_set_ana_cur_level(ch, aim_cur_level);
}

void lpctmu_vsel_isel_trim(void)
{
    SFR(P11_LPCTM0->ANA1, 3, 1, 1);//软件控制
    SFR(P11_LPCTM0->ANA1, 2, 1, 1);//模拟模块偏置使能
    SFR(P11_LPCTM0->ANA0, 0, 1, 1);//模拟模块使能
    SFR(P11_LPCTM0->ANA1, 4, 4, 0);//先不使能任何通道

    lpctmu_vsel_trim();

    __this->ch_idx = 0;
__isel_trim:

    lpctmu_isel_trim(__this->ch_list[__this->ch_idx]);
    __this->ch_idx ++;
    if (__this->ch_idx < __this->ch_num) {
        goto __isel_trim;
    } else {
        __this->ch_idx = 0;
    }
}

void lpctmu_lptimer_disable(void)
{
    P11_LPTMR3->CON0 &= ~BIT(0);
}

void lpctmu_lptimer_enable(void)
{
    P11_LPTMR3->CON0 |=  BIT(0);
    P11_LPTMR3->CON2 |=  BIT(8);
}

u32 lpctmu_lptimer_is_working(void)
{
    return (!!(P11_LPTMR3->CON0 & BIT(0)));
}

void lpctmu_lptimer_init(u32 scan_time)
{
    P11_LPTMR3->CON0 = 0x50;											//clr lc pending

    lpctmu_send_m2p_cmd(REQUEST_LPTMR_IRQ);

    u32 lrc_clk = __get_lrc_hz();
    P11_LPTMR3->CON2 = 0x03;								            //sel lrc; div1
    P11_LPTMR3->PRD = scan_time * lrc_clk / 1000;
    P11_LPTMR3->CON1 = 0x50;											//clr ex pending
    P11_LPTMR3->CON0 = 0x5b;											//clr lc pending | lc_ie1 | tmr_ctu | tmr_en

    P11_LPTMR3->CON2 |= BIT(8);
}

void lpctmu_dump(void)
{
    log_debug("P11_LPCTM0->CON0           = 0x%x", P11_LPCTM0->CON0);
    log_debug("P11_LPCTM0->CHEN           = 0x%x", P11_LPCTM0->CHEN);
    log_debug("P11_LPCTM0->CNUM           = 0x%x", P11_LPCTM0->CNUM);
    log_debug("P11_LPCTM0->PPRD           = 0x%x", P11_LPCTM0->PPRD);
    log_debug("P11_LPCTM0->DPRD           = 0x%x", P11_LPCTM0->DPRD);
    log_debug("P11_LPCTM0->ECON           = 0x%x", P11_LPCTM0->ECON);
    log_debug("P11_LPCTM0->EXEN           = 0x%x", P11_LPCTM0->EXEN);
    log_debug("P11_LPCTM0->CHIS           = 0x%x", P11_LPCTM0->CHIS);
    log_debug("P11_LPCTM0->CLKC           = 0x%x", P11_LPCTM0->CLKC);
    log_debug("P11_LPCTM0->WCON           = 0x%x", P11_LPCTM0->WCON);
    log_debug("P11_LPCTM0->ANA0           = 0x%x", P11_LPCTM0->ANA0);
    log_debug("P11_LPCTM0->ANA1           = 0x%x", P11_LPCTM0->ANA1);
    log_debug("P11_LPCTM0->RES            =   %d", P11_LPCTM0->RES);
    log_debug("P11_LPCTM0->DMA_START_ADR  =   %d", P11_LPCTM0->DMA_START_ADR);
    log_debug("P11_LPCTM0->DMA_HALF_ADR   =   %d", P11_LPCTM0->DMA_HALF_ADR);
    log_debug("P11_LPCTM0->DMA_END_ADR    =   %d", P11_LPCTM0->DMA_END_ADR);
    log_debug("P11_LPCTM0->DMA_CON        = 0x%x", P11_LPCTM0->DMA_CON);
    log_debug("P11_LPCTM0->MSG_CON        = 0x%x", P11_LPCTM0->MSG_CON);
    log_debug("P11_LPCTM0->DMA_WADR       = 0x%x", P11_LPCTM0->DMA_WADR);
    log_debug("P11_LPCTM0->SLEEP_CON      = 0x%x", P11_LPCTM0->SLEEP_CON);
}

void lpctmu_init(struct lpctmu_config_data *cfg_data)
{
    __this = cfg_data;
    if (!__this) {
        return;
    }

    u32 ch_idx, ch;
    u8 ch_en = 0;

    for (ch_idx = 0; ch_idx < __this->ch_num; ch_idx ++) {
        ch = __this->ch_list[ch_idx];
        ch_en |= BIT(ch);
        lpctmu_port_init(ch);
    }

    M2P_CTMU_CH_ENABLE = ch_en;
    M2P_CTMU_CH_WAKEUP_EN = __this->ch_wkp_en;
    M2P_CTMU_SCAN_TIME = __this->pdata->sample_scan_time;
    M2P_CTMU_LOWPOER_SCAN_TIME = __this->pdata->lowpower_sample_scan_time;

    if (!is_wakeup_source(PWR_WK_REASON_P11)) {

        lpctmu_lptimer_disable();
        memset(P11_LPCTM0, 0, sizeof(P11_LPCTM_TypeDef));

        lpctmu_send_m2p_cmd(REQUEST_LPCTMU_IRQ);

        //时钟源配置
        u32 lpctmu_clk = __get_lrc_hz();
        log_debug("lpctmu_clk = %dHz", lpctmu_clk);
        SFR(P11_LPCTM0->CLKC, 0, 2, 0);  //ctm_clk_p sel lrc200KHz
        //设置分频
        SFR(P11_LPCTM0->CLKC, 6, 1, 0); //divB = 1分频
        SFR(P11_LPCTM0->CLKC, 7, 1, 0); //divC = 1分频
        SFR(P11_LPCTM0->CLKC, 3, 3, 2); //div  = 2^2 = 4分频
        /**********************/
        //通道采集前的待稳定时间配置
        SFR(P11_LPCTM0->PPRD, 4, 4, 9);      //prp_prd = (9 + 1) * t 约等 50us > 10us
        //多通道轮询采集, 切通道时, 通道与通道的间隙时间配置
        SFR(P11_LPCTM0->PPRD, 0, 4, 9);      //stop_prd= (9 + 1) * t 约等 50us > 10us

        //每个通道采集的周期，常设几个毫秒
        u8 det_prd = __this->pdata->sample_window_time * lpctmu_clk / 4 / 1000 - 1;
        SFR(P11_LPCTM0->DPRD, 0, 8, det_prd);

        SFR(P11_LPCTM0->CON0, 2, 1, 1);      //LPCTM WKUP en
        SFR(P11_LPCTM0->CON0, 4, 1, 1);      //模拟滤波使能
        SFR(P11_LPCTM0->CON0, 1, 1, 0);      //模块的中断不使能
        SFR(P11_LPCTM0->CON0, 0, 1, 1);      //模块总开关

        lpctmu_vsel_isel_trim();

        SFR(P11_LPCTM0->ANA0, 0, 1, 0);
        SFR(P11_LPCTM0->ANA1, 3, 1, 0);
        SFR(P11_LPCTM0->ANA1, 2, 1, 0);

        SFR(P11_LPCTM0->CHEN, 0, 5, ch_en);

        if (__this->pdata->ext_stop_ch_en) {
            SFR(P11_LPCTM0->EXEN, 0, 5, __this->pdata->ext_stop_ch_en);
            SFR(P11_LPCTM0->ECON, 2, 2, __this->pdata->ext_stop_sel);
            SFR(P11_LPCTM0->ECON, 0, 1, 1);//与蓝牙的互斥使能

            extern void set_bt_wl_rab_wl2ext(u8 wl2ext_act0, u8 wl2ext_act1);
            set_bt_wl_rab_wl2ext(RF_TX_LDO, RF_TX_EN);
        }

        SFR(P11_LPCTM0->WCON, 6, 1, 1);     //clear pnd
        SFR(P11_LPCTM0->CON0, 0, 1, 0);     //模块先关闭
        SFR(P11_LPCTM0->CON0, 1, 1, 1);     //模块的RES中断使能

        lpctmu_lptimer_init(__this->pdata->sample_scan_time);

    } else {

        if (0 == lpctmu_lptimer_is_working()) {
            lpctmu_lptimer_init(__this->pdata->sample_scan_time);
        }
    }

    lpctmu_dump();
}

void lpctmu_disable(void)
{
    lpctmu_lptimer_disable();
}

void lpctmu_enable(void)
{
    lpctmu_lptimer_enable();
}

u32 lpctmu_is_sf_keep(void)
{
    /* extern void pvdd_output(u32 output); */
    /* pvdd_output(1); */
    if (!__this) {
        return 0;
    }
    if (lpctmu_lptimer_is_working()) {
        if (__this->softoff_keep_work == 0) {
            lpctmu_disable();
            return 0;
        } else {
            return 1;
        }
    }
    return 0;
}

