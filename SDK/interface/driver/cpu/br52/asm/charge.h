#ifndef _CHARGE_H_
#define _CHARGE_H_

#include "typedef.h"
#include "device.h"

/*------充满电电压选择 4.041V-4.534V-------*/
//低压压电池配置0~15
#define CHARGE_FULL_V_4041		0
#define CHARGE_FULL_V_4061		1
#define CHARGE_FULL_V_4081		2
#define CHARGE_FULL_V_4101		3
#define CHARGE_FULL_V_4119		4
#define CHARGE_FULL_V_4139		5
#define CHARGE_FULL_V_4159		6
#define CHARGE_FULL_V_4179		7
#define CHARGE_FULL_V_4199		8
#define CHARGE_FULL_V_4219		9
#define CHARGE_FULL_V_4238		10
#define CHARGE_FULL_V_4258		11
#define CHARGE_FULL_V_4278		12
#define CHARGE_FULL_V_4298		13
#define CHARGE_FULL_V_4318		14
#define CHARGE_FULL_V_4338		15
//高压电池配置16~31
#define CHARGE_FULL_V_4237		16
#define CHARGE_FULL_V_4257		17
#define CHARGE_FULL_V_4275		18
#define CHARGE_FULL_V_4295		19
#define CHARGE_FULL_V_4315		20
#define CHARGE_FULL_V_4335		21
#define CHARGE_FULL_V_4354		22
#define CHARGE_FULL_V_4374		23
#define CHARGE_FULL_V_4394		24
#define CHARGE_FULL_V_4414		25
#define CHARGE_FULL_V_4434		26
#define CHARGE_FULL_V_4454		27
#define CHARGE_FULL_V_4474		28
#define CHARGE_FULL_V_4494		29
#define CHARGE_FULL_V_4514		30
#define CHARGE_FULL_V_4534		31
#define CHARGE_FULL_V_MAX       32

/*充满判断电流为恒流电流的比例配置*/
#define CHARGE_FC_IS_CC_DIV_5		0 // full current = constant current / 5
#define CHARGE_FC_IS_CC_DIV_10		1 // default
#define CHARGE_FC_IS_CC_DIV_12P5	2
#define CHARGE_FC_IS_CC_DIV_15		3

/* 充电口下拉电阻 50k ~ 200k */
#define CHARGE_PULLDOWN_50K     0
#define CHARGE_PULLDOWN_100K    1
#define CHARGE_PULLDOWN_150K    2
#define CHARGE_PULLDOWN_200K    3

#define CHARGE_CCVOL_V			300		//涓流充电向恒流充电的转换点

#define DEVICE_EVENT_FROM_CHARGE	(('C' << 24) | ('H' << 16) | ('G' << 8) | '\0')

#define TRICKLE_EN_FLAG         BIT(9)  //判断是否开启涓流充电使能标志位

struct charge_platform_data {
    u8 charge_en;	        //内置充电使能
    u8 charge_poweron_en;	//开机充电使能
    u8 charge_full_V;	    //充满电电压大小
    u8 charge_full_mA;	    //充满电电流大小
    u16 charge_mA;	        //恒流充电电流大小
    u16 charge_trickle_mA;  //涓流充电电流大小
    u8 ldo5v_pulldown_en;   //下拉使能位
    u8 ldo5v_pulldown_lvl;	//ldo5v的下拉电阻配置项,若充电舱需要更大的负载才能检测到插入时，请将该变量置为对应阻值
    u8 ldo5v_pulldown_keep; //下拉电阻在softoff时是否保持,ldo5v_pulldown_en=1时有效
    u16 ldo5v_off_filter;	//ldo5v拔出过滤值，过滤时间 = (filter*2 + 20)ms,ldoin<0.6V且时间大于过滤时间才认为拔出,对于充满直接从5V掉到0V的充电仓，该值必须设置成0，对于充满由5V先掉到0V之后再升压到xV的充电仓，需要根据实际情况设置该值大小
    u16 ldo5v_on_filter;    //ldo5v>vbat插入过滤值,电压的过滤时间 = (filter*2)ms
    u16 ldo5v_keep_filter;  //1V<ldo5v<vbat维持电压过滤值,过滤时间= (filter*2)ms
    u16 charge_full_filter; //充满过滤值,连续检测充满信号恒为1才认为充满,过滤时间 = (filter*2)ms
};

#define CHARGE_PLATFORM_DATA_BEGIN(data) \
		struct charge_platform_data data  = {

#define CHARGE_PLATFORM_DATA_END()  \
};


enum {
    CHARGE_EVENT_CHARGE_START,
    CHARGE_EVENT_CHARGE_CLOSE,
    CHARGE_EVENT_CHARGE_FULL,
    CHARGE_EVENT_LDO5V_KEEP,
    CHARGE_EVENT_LDO5V_IN,
    CHARGE_EVENT_LDO5V_OFF,
};


void set_charge_event_flag(u8 flag);
void set_charge_online_flag(u8 flag);
void set_charge_event_flag(u8 flag);
u8 get_charge_online_flag(void);
u8 get_charge_poweron_en(void);
void set_charge_poweron_en(u32 onOff);
void charge_start(void);
void charge_close(void);
u16 get_charge_mA_config(void);
void set_charge_mA(u16 charge_mA);
u8 get_ldo5v_pulldown_en(void);
u8 get_ldo5v_pulldown_res(void);
u8 get_ldo5v_online_hw(void);
u8 get_lvcmp_det(void);
void charge_check_and_set_pinr(u8 mode);
u16 get_charge_full_value(void);
void charge_module_stop(void);
void charge_module_restart(void);
void ldoin_wakeup_isr(void);
void charge_wakeup_isr(void);
int charge_init(const struct charge_platform_data *data);
void charge_set_ldo5v_detect_stop(u8 stop);
u8 check_pinr_shutdown_enable(void);

#endif    //_CHARGE_H_
