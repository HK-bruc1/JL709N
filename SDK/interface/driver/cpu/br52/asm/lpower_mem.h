#ifndef _LOW_POWER_MEM_H_
#define _LOW_POWER_MEM_H_


#define SRAM_TEST_TYPE    5  //0:16K,1:8K,3:都关闭低功耗(会计数),4:都开启低功耗,5:循环测试开启和关闭低功耗

enum SRAM_MODE : u8 {
    NONE_MODE = 0,
    LS_MODE = 1,
    DS_MODE,
    SD_MODE,
    LS_DS_MODE,
};
enum SRAM_TYPE : u8 {
    SRAM_8K = 1,
    SRAM_64K = 2,
};

enum SRAM_UNIT : u8 {
    MEMORY_RAM0_0 = 0,
    MEMORY_RAM0_1,
    MEMORY_RAM0_2,
    MEMORY_RAM0_3,
    MEMORY_RAM0_4,
    MEMORY_RAM0_5,
    MEMORY_RAM0_6,
    MEMORY_RAM0_7,
    MEMORY_RAM1_0,
    MEMORY_RAM1_1,
    MEMORY_RAM1_2,
    MEMORY_RAM1_3,
    MEMORY_RAM1_4,
    MEMORY_RAM1_5,
    MEMORY_RAM1_6,
    MEMORY_RAM1_7,
    MEMORY_RAM2,
    MEMORY_RAM3,
    MEMORY_RAM4,
    MEMORY_RAM5,
};

struct ram_prd {
    // LS MODE:need cfg lsi、ceo
    u8 lsi;
    u8 ceo;
    // DS MODE:need cfg ceo、dso、dsi、rmo、rmi
    u8 dso;
    u8 dsi;
    u8 rmo;
    u8 rmi;
};

enum DEV_MEM : u32 {
    DEV_SBC       = BIT(0) | BIT(28),
    DEV_ANC_HD1   = BIT(1) | BIT(28),
    DEV_ANC_HD0   = BIT(2) | BIT(28),
    DEV_ANC_AFC1  = BIT(3) | BIT(28),
    DEV_ANC_AFC0  = BIT(4) | BIT(28),
    DEV_ANC_SZ1   = BIT(5) | BIT(28),
    DEV_ANC_SZ0   = BIT(6) | BIT(28),
    DEV_ANC_FB1   = BIT(7) | BIT(28),
    DEV_ANC_FB0   = BIT(8) | BIT(28),
    DEV_ANC_FF1   = BIT(9) | BIT(28),
    DEV_ANC_FF0   = BIT(10) | BIT(28),
    DEV_DAC       = BIT(11) | BIT(28),
    DEV_ADC0      = BIT(12) | BIT(28),
    DEV_ADC1      = BIT(13) | BIT(28),
    DEV_ADC2      = BIT(14) | BIT(28),

    DEV_EQ        = BIT(16) | BIT(29),
    DEV_SRC0_FLTB = BIT(17) | BIT(29),
    DEV_SRC0_USF  = BIT(18) | BIT(29),
    DEV_SRC1_RAM0 = BIT(19) | BIT(29),
    DEV_SRC1_RAM1 = BIT(20) | BIT(29),

    DEV_AGC_RAM   = BIT(25) | BIT(30),
    DEV_ANA_RAM   = BIT(26) | BIT(30),
    DEV_SRAM0     = BIT(27) | BIT(30),
};

enum DEV_MODE : u8 {
    DEV_MODE_PSD = 0,  // power shut down, 数据丢失, 掉电时读取数据为0
    DEV_MODE_RET,      // retention 深度睡眠, 数据保持, 睡眠时读取数据为0
    DEV_MODE_NAP,      // nap 浅睡眠, 数据保持, 浅睡眠时可以读数据(获取到的数据与进入睡眠前一次读取的数据相同), 写数据无效
};

#define LP01_CNT(y,x)   (JL_LPRAM->LPRAM##x##_0_##y##CNT + JL_LPRAM->LPRAM##x##_1_##y##CNT + JL_LPRAM->LPRAM##x##_2_##y##CNT + JL_LPRAM->LPRAM##x##_3_##y##CNT + \
                         JL_LPRAM->LPRAM##x##_4_##y##CNT + JL_LPRAM->LPRAM##x##_5_##y##CNT + JL_LPRAM->LPRAM##x##_6_##y##CNT + JL_LPRAM->LPRAM##x##_7_##y##CNT)

// #define LP25_CNT(y,x)   (JL_LPRAM->LPRAM##x##_##y##CNT + JL_LPRAM->LPRAM##x##_##y##CNT + JL_LPRAM->LPRAM##x##_##y##CNT + JL_LPRAM->LPRAM##x##_##y##CNT + \
//                          JL_LPRAM->LPRAM##x##_##y##CNT + JL_LPRAM->LPRAM##x##_##y##CNT + JL_LPRAM->LPRAM##x##_##y##CNT + JL_LPRAM->LPRAM##x##_##y##CNT)

#define LP25_CNT(y,x)   (JL_LPRAM->LPRAM##x##_##y##CNT)

#define LP01_RWCNT      /* (u32) */(LP01_CNT(RW,0) + LP01_CNT(RW,1))
#define LP25_RWCNT      /* (u32) */(LP25_CNT(RW,2) + LP25_CNT(RW,3) + LP25_CNT(RW,4) + LP25_CNT(RW,5))

#define LP01_LPCNT      /* (u32) */(LP01_CNT(LP,0) + LP01_CNT(LP,1))
#define LP25_LPCNT      /* (u32) */(LP25_CNT(LP,2) + LP25_CNT(LP,3) + LP25_CNT(LP,4) + LP25_CNT(LP,5))

#define LP01_ITCNT      /* (u32) */(LP01_CNT(IT,0) + LP01_CNT(IT,1))
#define LP25_ITCNT      /* (u32) */(LP25_CNT(IT,2) + LP25_CNT(IT,3) + LP25_CNT(IT,4) + LP25_CNT(IT,5))

#define LP01_WTCNT      /* (u32) */(LP01_CNT(WT,0) + LP01_CNT(WT,1))
#define LP25_WTCNT      /* (u32) */(LP25_CNT(WT,2) + LP25_CNT(WT,3) + LP25_CNT(WT,4) + LP25_CNT(WT,5))

#define LP_LPCNT        (LP01_LPCNT + LP25_LPCNT)
#define LP_RWCNT        (LP01_RWCNT + LP25_RWCNT)
#define LP_ITCNT        (LP01_ITCNT + LP25_ITCNT)
#define LP_WTCNT        (LP01_WTCNT + LP25_WTCNT)


void sram_sd_mode_init(u32 idle_ram);
void sram_enter_sd_mode(u32 idle_ram);
void sram_exit_sd_mode(u32 will_used_ram);

void dev_mem_exit_low_power(enum DEV_MEM dev, enum DEV_MODE mode);
void dev_mem_enter_low_power(enum DEV_MEM dev, enum DEV_MODE mode);

#endif

