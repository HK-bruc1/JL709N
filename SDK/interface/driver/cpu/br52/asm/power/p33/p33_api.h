#ifndef __P33_API_H__
#define __P33_API_H__


//
//
//					vol
//
//
//
/****************************************************************/

enum DVDD_VOL {
    DVDD_VOL_0625V = 0,
    DVDD_VOL_0650V,
    DVDD_VOL_0675V,
    DVDD_VOL_0700V,
    DVDD_VOL_0725V,
    DVDD_VOL_0750V,
    DVDD_VOL_0775V,
    DVDD_VOL_0800V,
    DVDD_VOL_0825V,
    DVDD_VOL_0850V,
    DVDD_VOL_0875V,
    DVDD_VOL_0900V,
    DVDD_VOL_0925V,
    DVDD_VOL_0950V,
    DVDD_VOL_0975V,
    DVDD_VOL_1000V,
};
/*
enum DVDD2_VOL {
    DVDD2_VOL_0675V = 0,
    DVDD2_VOL_0700V,
    DVDD2_VOL_0725V,
    DVDD2_VOL_0750V,
    DVDD2_VOL_0775V,
    DVDD2_VOL_0800V,
    DVDD2_VOL_0825V,
    DVDD2_VOL_0850V,
    DVDD2_VOL_0875V,
    DVDD2_VOL_0900V,
    DVDD2_VOL_0925V,
    DVDD2_VOL_0950V,
    DVDD2_VOL_0975V,
    DVDD2_VOL_1000V,
    DVDD2_VOL_1025V,
    DVDD2_VOL_1050V,
};
*/
enum RVDD_VOL {
    RVDD_VOL_0625V = 0,
    RVDD_VOL_0650V,
    RVDD_VOL_0675V,
    RVDD_VOL_0700V,
    RVDD_VOL_0725V,
    RVDD_VOL_0750V,
    RVDD_VOL_0775V,
    RVDD_VOL_0800V,
    RVDD_VOL_0825V,
    RVDD_VOL_0850V,
    RVDD_VOL_0875V,
    RVDD_VOL_0900V,
    RVDD_VOL_0925V,
    RVDD_VOL_0950V,
    RVDD_VOL_0975V,
    RVDD_VOL_1000V,
};
/*
enum RVDD2_VOL {
    RVDD2_VOL_0675V = 0,
    RVDD2_VOL_0700V,
    RVDD2_VOL_0725V,
    RVDD2_VOL_0750V,
    RVDD2_VOL_0775V,
    RVDD2_VOL_0800V,
    RVDD2_VOL_0825V,
    RVDD2_VOL_0850V,
    RVDD2_VOL_0875V,
    RVDD2_VOL_0900V,
    RVDD2_VOL_0925V,
    RVDD2_VOL_0950V,
    RVDD2_VOL_0975V,
    RVDD2_VOL_1000V,
    RVDD2_VOL_1025V,
    RVDD2_VOL_1500V,
};

enum BTVDD_VOL {
    BTVDD_VOL_1625V = 0,
    BTVDD_VOL_1650V,
    BTVDD_VOL_1675V,
    BTVDD_VOL_1700V,
    BTVDD_VOL_1725V,
    BTVDD_VOL_1750V,
    BTVDD_VOL_1775V,
    BTVDD_VOL_1800V,
    BTVDD_VOL_1825V,
    BTVDD_VOL_1850V,
    BTVDD_VOL_1900V,
    BTVDD_VOL_1925V,
    BTVDD_VOL_1950V,
    BTVDD_VOL_1975V,
    BTVDD_VOL_2000V,
};
*/
enum DCVDD_VOL {
    DCVDD_VOL_1000V = 0,
    DCVDD_VOL_1025V,
    DCVDD_VOL_1050V,
    DCVDD_VOL_1075V,
    DCVDD_VOL_1100V,
    DCVDD_VOL_1125V,
    DCVDD_VOL_1150V,
    DCVDD_VOL_1175V,
    DCVDD_VOL_1200V,
    DCVDD_VOL_1225V,
    DCVDD_VOL_1250V,
    DCVDD_VOL_1275V,
    DCVDD_VOL_1300V,
    DCVDD_VOL_1325V,
    DCVDD_VOL_1350V,
    DCVDD_VOL_1375V,
};

enum VDDIOM_VOL {
    VDDIOM_VOL_21V = 0,
    VDDIOM_VOL_22V,
    VDDIOM_VOL_23V,
    VDDIOM_VOL_24V,
    VDDIOM_VOL_25V,
    VDDIOM_VOL_26V,
    VDDIOM_VOL_27V,
    VDDIOM_VOL_28V,
    VDDIOM_VOL_29V,
    VDDIOM_VOL_30V,
    VDDIOM_VOL_31V,
    VDDIOM_VOL_32V,
    VDDIOM_VOL_33V,
    VDDIOM_VOL_34V,
    VDDIOM_VOL_35V,
    VDDIOM_VOL_36V,
};

enum VDDIOM_VOL_L {
    VDDIOM_VOL_17V_L = 0,
    VDDIOM_VOL_18V_L,
    VDDIOM_VOL_19V_L,
    VDDIOM_VOL_20V_L,
    VDDIOM_VOL_21V_L,
    VDDIOM_VOL_22V_L,
    VDDIOM_VOL_23V_L,
    VDDIOM_VOL_24V_L,
    VDDIOM_VOL_25V_L,
    VDDIOM_VOL_26V_L,
    VDDIOM_VOL_27V_L,
    VDDIOM_VOL_28V_L,
    VDDIOM_VOL_29V_L,
    VDDIOM_VOL_30V_L,
    VDDIOM_VOL_31V_L,
    VDDIOM_VOL_32V_L,
};

enum VDDIOW_VOL {
    VDDIOW_VOL_21V = 0,
    VDDIOW_VOL_22V,
    VDDIOW_VOL_23V,
    VDDIOW_VOL_24V,
    VDDIOW_VOL_25V,
    VDDIOW_VOL_26V,
    VDDIOW_VOL_27V,
    VDDIOW_VOL_28V,
    VDDIOW_VOL_29V,
    VDDIOW_VOL_30V,
    VDDIOW_VOL_31V,
    VDDIOW_VOL_32V,
    VDDIOW_VOL_33V,
    VDDIOW_VOL_34V,
    VDDIOW_VOL_35V,
    VDDIOW_VOL_36V,
};

enum VDDIOW_VOL_L {
    VDDIOW_VOL_17V_L = 0,
    VDDIOW_VOL_18V_L,
    VDDIOW_VOL_19V_L,
    VDDIOW_VOL_20V_L,
    VDDIOW_VOL_21V_L,
    VDDIOW_VOL_22V_L,
    VDDIOW_VOL_23V_L,
    VDDIOW_VOL_24V_L,
    VDDIOW_VOL_25V_L,
    VDDIOW_VOL_26V_L,
    VDDIOW_VOL_27V_L,
    VDDIOW_VOL_28V_L,
    VDDIOW_VOL_29V_L,
    VDDIOW_VOL_30V_L,
    VDDIOW_VOL_31V_L,
    VDDIOW_VOL_32V_L,
};



enum WVDD_VOL {
    WVDD_VOL_040V = 0,
    WVDD_VOL_045V,
    WVDD_VOL_050V,
    WVDD_VOL_055V,
    WVDD_VOL_060V,
    WVDD_VOL_065V,
    WVDD_VOL_070V,
    WVDD_VOL_075V,
    WVDD_VOL_080V,
    WVDD_VOL_085V,
    WVDD_VOL_090V,
    WVDD_VOL_095V,
    WVDD_VOL_100V,
    WVDD_VOL_105V,
    WVDD_VOL_110V,
    WVDD_VOL_115V,
};

enum PVDD_VOL {
    PVDD_VOL_040V = 0,
    PVDD_VOL_045V,
    PVDD_VOL_050V,
    PVDD_VOL_055V,
    PVDD_VOL_060V,
    PVDD_VOL_065V,
    PVDD_VOL_070V,
    PVDD_VOL_075V,
    PVDD_VOL_080V,
    PVDD_VOL_085V,
    PVDD_VOL_090V,
    PVDD_VOL_095V,
    PVDD_VOL_100V,
    PVDD_VOL_105V,
    PVDD_VOL_110V,
    PVDD_VOL_115V,
};

void dvdd_vol_sel(enum DVDD_VOL vol);
enum DVDD_VOL get_dvdd_vol_sel();
//void dvdd2_vol_sel(enum DVDD2_VOL vol);
//enum DVDD2_VOL get_dvdd2_vol_sel();

void rvdd_vol_sel(enum RVDD_VOL vol);
enum RVDD_VOL get_rvdd_vol_sel();
//void rvdd2_vol_sel(enum RVDD2_VOL vol);
//enum RVDD2_VOL get_rvdd2_vol_sel();

void dcvdd_vol_sel(enum DCVDD_VOL vol);
enum DCVDD_VOL get_dcvdd_vol_sel();

//void btvdd_vol_sel(enum BTVDD_VOL vol);
//enum BTVDD_VOL get_btvdd_vol_sel();

void pvdd_config(u32 lev, u32 low_lev, u32 output);
void pvdd_output(u32 output);

void vddiom_vol_sel(enum VDDIOM_VOL vol);
enum VDDIOM_VOL get_vddiom_vol_sel();
void vddiow_vol_sel(enum VDDIOW_VOL vol);
enum VDDIOW_VOL get_vddiow_vol_sel();
u32 get_vddiom_vol();

//
//
//					lvd
//
//
//
/****************************************************************/
typedef enum {
    LVD_RESET_MODE,		//复位模式
    LVD_EXCEPTION_MODE, //异常模式，进入异常中断
    LVD_WAKEUP_MODE,    //唤醒模式，进入唤醒中断，callback参数为回调函数
} LVD_MODE;

typedef enum {
    VLVD_SEL_15V = 0,
    VLVD_SEL_16V,
    VLVD_SEL_17V,
    VLVD_SEL_18V,
    VLVD_SEL_19V,
    VLVD_SEL_20V,
    VLVD_SEL_21V,
    VLVD_SEL_22V,
    VLVD_SEL_23V,
    VLVD_SEL_24V,
    VLVD_SEL_25V,
    VLVD_SEL_26V,
    VLVD_SEL_27V,
    VLVD_SEL_28V,
    VLVD_SEL_29V,
    VLVD_SEL_30V,
} LVD_VOL;

void lvd_en(u8 en);
void lvd_config(LVD_VOL vol, u8 expin_en, LVD_MODE mode, void (*callback));

//
//
//                    pinr
//
//
//
//******************************************************************
void gpio_longpress_pin0_reset_config(u32 pin, u32 level, u32 time, u32 release, u32 pull_enable);
void gpio_longpress_pin1_reset_config(u32 pin, u32 level, u32 time, u32 release);



//
//
//                    dcdc
//
//
//
//******************************************************************
enum POWER_MODE {
    //LDO模式
    PWR_LDO15,
    //DCDC模式
    PWR_DCDC15,
};

enum POWER_DCDC_TYPE {
    PWR_DCDC12 = 2,
    PWR_DCDC12_DCDC09 = 3,
};

enum {
    DCDC09 = 1,
    DCDC12 = 2,
    DCDC18 = 4,
};

void power_set_dcdc_type(enum POWER_DCDC_TYPE type);
void power_set_mode(enum POWER_MODE mode);

#define MAX_WAKEUP_PORT     8  //最大同时支持数字io输入个数
#define MAX_WAKEUP_ANA_PORT 3  //最大同时支持模拟io输入个数

#endif
