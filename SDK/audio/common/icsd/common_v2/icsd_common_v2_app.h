#ifndef _ICSD_COMMON_V2_APP_H_
#define _ICSD_COMMON_V2_APP_H_

#include "generic/typedef.h"
#include "icsd_common_v2.h"

/*AFQ COMMON MSG List*/
enum {
    AFQ_COMMON_MSG_RUN = 0xA1,
};

enum {
    ANC_EXT_OPEN_FAIL_REENTRY = 0x1,	//重入
    ANC_EXT_OPEN_FAIL_FUNC_CONFLICT,	//功能冲突
    ANC_EXT_OPEN_FAIL_CFG_MISS,			//参数缺失

    ANC_EXT_OPEN_FAIL_NO_ANC_MODE, 		//非ANC模式
    ANC_EXT_OPEN_FAIL_SWITCH_LOCK, 		//切模式锁存
    ANC_EXT_OPEN_FAIL_AFQ_RUNNING, 		//AFQ运行中
    ANC_EXT_OPEN_FAIL_MAX_IIR_LIMIT, 	//最大滤波器限制
    ANC_EXT_OPEN_FAIL_RTANC_CFG_MISS, 	//缺失RTANC参数
};

//频响获取选择
enum audio_adaptive_fre_sel {
    AUDIO_ADAPTIVE_FRE_SEL_AFQ = BIT(0),
    AUDIO_ADAPTIVE_FRE_SEL_ANC = BIT(1),
};

//AEQ / CMP 更新标志
enum audio_adaptive_update_flag {
    AUDIO_ADAPTIVE_AEQ_UPDATE_FLAG = BIT(0),
    AUDIO_ADAPTIVE_CMP_UPDATE_FLAG = BIT(1),
};

enum audio_afq_priority {
    AUDIO_AFQ_PRIORITY_NORMAL = 0,         //普通模式，不同业务场景可自加互斥
    AUDIO_AFQ_PRIORITY_HIGH,            //高优先级模式，任意场景必须执行, 一般用于入耳快速收敛
};

//APP存储频响结构
struct audio_afq_output {
    u8 state;
    u8 priority;
    u8 cmp_eq_update;
    u8 sz_l_sel_idx;
    __afq_data sz_l;
    __afq_data sz_r;
};

struct audio_afq_bulk {
    struct list_head entry;
    const char *name;
    void (*output)(struct audio_afq_output *p);
};

int audio_afq_common_init(void);
int audio_afq_common_del_output_handler(const char *name);
int audio_afq_common_add_output_handler(const char *name, int data_sel, void (*output)(struct audio_afq_output *p));
int audio_afq_common_output_post_msg(__afq_output *out, u8 priority);
//判断AFQ APP应用是否活动
u8 audio_afq_common_app_is_active(void);
void audio_afq_common_sz_sel_idx_set(u8 idx);
void audio_afq_common_cmp_eq_update_set(u8 cmp_eq_update);

#endif
