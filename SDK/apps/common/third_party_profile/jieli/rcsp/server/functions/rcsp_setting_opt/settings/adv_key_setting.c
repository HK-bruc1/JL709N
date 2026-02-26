#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".adv_key_setting.data.bss")
#pragma data_seg(".adv_key_setting.data")
#pragma const_seg(".adv_key_setting.text.const")
#pragma code_seg(".adv_key_setting.text")
#endif
#include "app_config.h"
#include "syscfg_id.h"
#include "key_event_deal.h"
#include "ble_rcsp_server.h"

#include "rcsp_setting_sync.h"
#include "rcsp_setting_opt.h"
#include "adv_anc_voice_key.h"
#include "app_msg.h"
#include "key_driver.h"
#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif

#include "customer.h"
#include "btstack/avctp_user.h"

#if (RCSP_MODE && RCSP_ADV_EN)
#if RCSP_ADV_KEY_SET_ENABLE

extern int get_bt_tws_connect_status();
static u8 rcsp_adv_key_event_flag = 0x0;

#define ADV_POWER_ON_OFF	0
enum RCSP_KEY_TYPE {
    RCSP_KEY_TYPE_NULL = 0x00,
#if ADV_POWER_ON_OFF
    RCSP_KEY_TYPE_POWER_ON,
    RCSP_KEY_TYPE_POWER_OFF,
#endif
    RCSP_KEY_TYPE_PREV = 0x03,
    RCSP_KEY_TYPE_NEXT,
    RCSP_KEY_TYPE_PP,
    RCSP_KEY_TYPE_ANSWER_CALL,
    RCSP_KEY_TYPE_HANG_UP,
    RCSP_KEY_TYPE_CALL_BACK,
    RCSP_KEY_TYPE_INC_VOICE,
    RCSP_KEY_TYPE_DESC_VOICE,
    RCSP_KEY_TYPE_TAKE_PHOTO,
    RCSP_ADV_KEY_TYPE_SIRI,
	RCSP_ADV_KEY_TYPE_LOW,
    RCSP_KEY_TYPE_ANC_VOICE = 0xFF,
};

enum RCSP_EAR_CHANNEL {
    RCSP_EAR_CHANNEL_LEFT = 0x01,
    RCSP_EAR_CHANNEL_RIGHT = 0x02,
};

// enum RCSP_KEY_ACTION {
//     RCSP_KEY_ACTION_CLICK = 0x01,
//     RCSP_KEY_ACTION_DOUBLE_CLICK = 0x02,
// 	RCSP_KEY_ACTION_LONG_CLICK = 0x03,
// 	RCSP_KEY_ACTION_THREE_CLICK = 0x04,
// };

enum RCSP_KEY_ACTION {
    _RCSP_KEY_ACTION_1 = 1,
    _RCSP_KEY_ACTION_2,
    _RCSP_KEY_ACTION_3,
    _RCSP_KEY_ACTION_4,
};

static u8 g_key_setting[24] = {
    RCSP_EAR_CHANNEL_LEFT,  RCSP_KEY_ACTION_CLICK,      _RCSP_KEY_ACTION_CLICK_LEFT, \
    RCSP_EAR_CHANNEL_RIGHT, RCSP_KEY_ACTION_CLICK,      _RCSP_KEY_ACTION_CLICK_RIGHT, \
    RCSP_EAR_CHANNEL_LEFT,  RCSP_KEY_ACTION_DOUBLE_CLICK, _RCSP_KEY_ACTION_DOUBLE_CLICK_LEFT, \
    RCSP_EAR_CHANNEL_RIGHT, RCSP_KEY_ACTION_DOUBLE_CLICK, _RCSP_KEY_ACTION_DOUBLE_CLICK_RIGHT, \
    RCSP_EAR_CHANNEL_LEFT,  RCSP_KEY_ACTION_LONG_CLICK, _RCSP_KEY_ACTION_LONG_CLICK_LEFT, \
    RCSP_EAR_CHANNEL_RIGHT, RCSP_KEY_ACTION_LONG_CLICK, _RCSP_KEY_ACTION_LONG_CLICK_RIGHT, \
    RCSP_EAR_CHANNEL_LEFT,  RCSP_KEY_ACTION_THREE_CLICK,  _RCSP_KEY_ACTION_THREE_CLICK_LEFT, \
    RCSP_EAR_CHANNEL_RIGHT, RCSP_KEY_ACTION_THREE_CLICK,  _RCSP_KEY_ACTION_THREE_CLICK_RIGHT
};
// 按键缓存
// 第一列 | 第二列 | 第三列
// 01 左耳| 01 单击| 按键功能
// 02 右耳| 02 双击| 按键功能

static void enable_adv_key_event(void)
{
    y_printf("%s() %d", __func__, __LINE__);
    rcsp_adv_key_event_flag = 1;
}

static u8 get_adv_key_event_status(void)
{
    y_printf("%s() %d", __func__, __LINE__);
    return rcsp_adv_key_event_flag;
}

static void disable_adv_key_event(void)
{
    y_printf("%s() %d", __func__, __LINE__);
    rcsp_adv_key_event_flag = 0;
}

static void set_key_setting(u8 *key_setting_info)
{
    y_printf("%s() %d", __func__, __LINE__);
    //这里应该是RCSP发送自定义按键命令过来修改缓存列表的
    // 设置左耳短按功能
    g_key_setting[3 * 0 + 2] = key_setting_info[0];
    // 获取右耳短按功能
    g_key_setting[3 * 1 + 2] = key_setting_info[1];
    // 获取左耳双击功能
    g_key_setting[3 * 2 + 2] = key_setting_info[2];
    // 获取右耳双击功能
    g_key_setting[3 * 3 + 2] = key_setting_info[3];
    // 获取左耳长按功能
    g_key_setting[3 * 4 + 2] = key_setting_info[4];
    // 获取右耳长按功能
    g_key_setting[3 * 5 + 2] = key_setting_info[5];
    // 获取左耳三击功能
    g_key_setting[3 * 6 + 2] = key_setting_info[6];
    // 获取右耳三击功能
    g_key_setting[3 * 7 + 2] = key_setting_info[7];
    y_printf("set_key_setting : %d %d %d %d %d %d %d %d\n",
           g_key_setting[2], g_key_setting[5], g_key_setting[8], g_key_setting[11],
           g_key_setting[14], g_key_setting[17], g_key_setting[20], g_key_setting[23]);

}

static int get_key_setting(u8 *key_setting_info)
{
    y_printf("%s() %d", __func__, __LINE__);
    // 获取左耳短按功能
    key_setting_info[0] = g_key_setting[3 * 0 + 2];
    // 获取右耳短按功能
    key_setting_info[1] = g_key_setting[3 * 1 + 2];
    // 获取左耳双击功能
    key_setting_info[2] = g_key_setting[3 * 2 + 2];
    // 获取右耳双击功能
    key_setting_info[3] = g_key_setting[3 * 3 + 2];

    //只要get和set一一对应就不会混乱,存入VM再取出VM
    // 获取左耳长按功能
    key_setting_info[4] = g_key_setting[3 * 4 + 2];
    // 获取右耳长按功能
    key_setting_info[5] = g_key_setting[3 * 5 + 2];
    // 获取左耳三击功能
    key_setting_info[6] = g_key_setting[3 * 6 + 2];
    // 获取右耳三击功能
    key_setting_info[7] = g_key_setting[3 * 7 + 2];
    y_printf("get_key_setting : %d %d %d %d %d %d %d %d\n",
           key_setting_info[0], key_setting_info[1], key_setting_info[2], key_setting_info[3],
           key_setting_info[4], key_setting_info[5], key_setting_info[6], key_setting_info[7]);
    return 0;
}

static void update_key_setting_vm_value(u8 *key_setting_info)
{
    y_printf("%s() %d", __func__, __LINE__);
    //实际写入长度需要跟缓存按键数量一致
    syscfg_write(CFG_RCSP_ADV_KEY_SETTING, key_setting_info, 8);
    // 打印：写入 VM 后的值（确认没被 syscfg_write 修改）
    y_printf("VM Write AFTER : %d %d %d %d %d %d %d %d\n",
           key_setting_info[0], key_setting_info[1], key_setting_info[2], key_setting_info[3],
           key_setting_info[4], key_setting_info[5], key_setting_info[6], key_setting_info[7]);
}

static void key_setting_sync(u8 *key_setting_info)
{
    y_printf("%s() %d", __func__, __LINE__);
    y_printf("key_setting_sync : %d %d %d %d %d %d %d %d\n",
           key_setting_info[0], key_setting_info[1], key_setting_info[2], key_setting_info[3],
           key_setting_info[4], key_setting_info[5], key_setting_info[6], key_setting_info[7]);
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        update_rcsp_setting(ATTR_TYPE_KEY_SETTING);
    }
#endif
}

static void deal_key_setting(u8 *key_setting_info, u8 write_vm, u8 tws_sync)
{
    y_printf("%s() %d", __func__, __LINE__);
    u8 key_setting[8];
    if (!key_setting_info) {
        get_key_setting(key_setting);
    } else {
        memcpy(key_setting, key_setting_info, 8);
        y_printf("deal_key_setting : %d %d %d %d %d %d %d %d\n",
           key_setting_info[0], key_setting_info[1], key_setting_info[2], key_setting_info[3],
           key_setting_info[4], key_setting_info[5], key_setting_info[6], key_setting_info[7]);
    }
    // 1、写入VM
    if (write_vm) {
        update_key_setting_vm_value(key_setting);
    }
    // 2、同步对端
    if (tws_sync) {
        key_setting_sync(key_setting);
    }
    // 3、更新状态
    enable_adv_key_event();
}


static u8 get_adv_key_opt(u8 key_action, u8 channel)
{
    y_printf("%s() %d", __func__, __LINE__);
    u8 opt;
    for (opt = 0; opt < sizeof(g_key_setting); opt += 3) {
        if (g_key_setting[opt] == channel &&
            g_key_setting[opt + 1] == key_action) {
            break;
        }
    }
    if (sizeof(g_key_setting) == opt) {
        return -1;
    }

    switch (g_key_setting[opt + 2]) {
    case RCSP_KEY_TYPE_NULL:
        opt = APP_MSG_NULL;
        break;
#if ADV_POWER_ON_OFF
    case RCSP_KEY_TYPE_POWER_ON:
        opt = APP_MSG_POWER_ON;
        break;
    case RCSP_KEY_TYPE_POWER_OFF:
        opt = APP_MSG_POWER_OFF;
        break;
#endif
    case RCSP_KEY_TYPE_PREV:
        opt = APP_MSG_MUSIC_PREV;
        y_printf("RCSP:RCSP_KEY_TYPE_PREV---->APP_MSG_MUSIC_PREV\n");
        break;
    case RCSP_KEY_TYPE_NEXT:
        opt = APP_MSG_MUSIC_NEXT;
        y_printf("RCSP:RCSP_KEY_TYPE_NEXT---->APP_MSG_MUSIC_NEXT\n");
        break;
    case RCSP_KEY_TYPE_PP:
        opt = APP_MSG_MUSIC_PP;
        y_printf("RCSP:RCSP_KEY_TYPE_PP---->APP_MSG_MUSIC_PP\n");
        break;
    case RCSP_KEY_TYPE_ANSWER_CALL:
        opt = APP_MSG_CALL_ANSWER;
        break;
    case RCSP_KEY_TYPE_HANG_UP:
        opt = APP_MSG_CALL_HANGUP;
        break;
    case RCSP_KEY_TYPE_CALL_BACK:
        opt = APP_MSG_CALL_LAST_NO;
        break;
    case RCSP_KEY_TYPE_INC_VOICE:
        opt = APP_MSG_VOL_UP;
        y_printf("RCSP:RCSP_KEY_TYPE_INC_VOICE---->APP_MSG_VOL_UP\n");
        break;
    case RCSP_KEY_TYPE_DESC_VOICE:
        opt = APP_MSG_VOL_DOWN;
        y_printf("RCSP:RCSP_KEY_TYPE_DESC_VOICE---->APP_MSG_VOL_DOWN\n");
        break;
  
    case RCSP_ADV_KEY_TYPE_SIRI:
        opt = APP_MSG_OPEN_SIRI;
        y_printf("RCSP:RCSP_ADV_KEY_TYPE_SIRI---->APP_MSG_OPEN_SIRI\n");
        break;  
    case RCSP_ADV_KEY_TYPE_LOW:
        opt = APP_MSG_LOW_LANTECY;
        y_printf("RCSP:RCSP_ADV_KEY_TYPE_LOW---->APP_MSG_LOW_LANTECY\n");
        break;  
    case RCSP_KEY_TYPE_ANC_VOICE:
        opt = APP_MSG_NULL;
#if (RCSP_ADV_EN && RCSP_ADV_ANC_VOICE)
#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            break;
        }
#endif
        update_anc_voice_key_opt();
#endif
        break;
    }
    return opt;
}

/**
 * rcsp按键配置转换
 *
 * @param value 按键功能
 * @param msg 按键消息
 *
 * @return 是否拦截消息
 */
int rcsp_key_event_remap(int *msg)
{
    y_printf("%s() %d", __func__, __LINE__);
#if _APP_KEY_CALL_ENABLE
    //通话状态下不进行拦截，因为APP中不涉及有关通话相关的自定义按键
    //避免出现来电时普通转换流程是单机接听，但是被RCSP拦截转换为了单击语音助手
    if((bt_get_call_status() == BT_CALL_INCOMING) || (bt_get_call_status() == BT_CALL_OUTGOING) || (bt_get_call_status() == BT_CALL_ACTIVE)){
        //此时不拦截，走普通按键转换APP层消息流程
        y_printf("RCSP检测到通话状态不拦截\n");
        return -1;
    }
#endif

    //参照普通按键流程，从机直接返回
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        //这里从机是直接返回的，那么所有的按键消息都是在主耳处理的。
        return -1;
    }

    //当开启rcsp时，没有进入APP改过按键的话，也不会走RCSP流程！！！
    if (0 == get_adv_key_event_status()) {
        y_printf("disable_adv_key_event\n");
        return -1;
    }
    int key_value = APP_MSG_KEY_VALUE(msg[0]);
    if (key_value != KEY_POWER) {
        return -1;
    }
    int key_action = APP_MSG_KEY_ACTION(msg[0]);

    switch (key_action) {
    //os从消息队列中取出按键消息转换为APP层消息时会调用这个
    //rcsp是APP自定义按键，开启APP的情况下先走这里，再走普通转换流程
    //通过这里转换后，得到RCSP按键类型与左右声道去缓存列表中遍历到真正的APP层按键处理消息
    case KEY_ACTION_CLICK:
        // 单击
        key_action = RCSP_KEY_ACTION_CLICK;
        y_printf("RCSP:KEY_ACTION_CLICK---->RCSP_KEY_ACTION_CLICK\n");
        break;
    case KEY_ACTION_DOUBLE_CLICK:
        // 双击
        key_action = RCSP_KEY_ACTION_DOUBLE_CLICK;
        y_printf("RCSP:KEY_ACTION_DOUBLE_CLICK---->RCSP_KEY_ACTION_DOUBLE_CLICK\n");
        break;
    case KEY_ACTION_TRIPLE_CLICK:
        //三击
        key_action = RCSP_KEY_ACTION_THREE_CLICK;
        y_printf("RCSP:KEY_ACTION_TRIPLE_CLICK---->RCSP_KEY_ACTION_THREE_CLICK\n");
        break;
    case KEY_ACTION_LONG:
        //长按
        key_action = RCSP_KEY_ACTION_LONG_CLICK;
        y_printf("RCSP:KEY_ACTION_LONG---->RCSP_KEY_ACTION_LONG_CLICK\n");
        break;
    default:
        return -1;
    }

#if (TCFG_USER_TWS_ENABLE)
    u8 channel = tws_api_get_local_channel();
#else
    u8 channel = 'U';
#endif

    // switch (channel) {
    // case 'U':
    // case 'L':
    //     channel = (msg[1] == APP_KEY_MSG_FROM_TWS) ? RCSP_EAR_CHANNEL_RIGHT : RCSP_EAR_CHANNEL_LEFT;
    //     break;
    // case 'R':
    //     channel = (msg[1] == APP_KEY_MSG_FROM_TWS) ? RCSP_EAR_CHANNEL_LEFT : RCSP_EAR_CHANNEL_RIGHT;
    //     break;
    // default:
    //     return -1;
    // }
    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
        //本地左耳是主机，消息不来自对耳，那么就是本地左耳的按键操作
        //如果右耳是主机，消息来自对耳，那么就是从机左耳的按键操作。
        channel = RCSP_EAR_CHANNEL_LEFT;
    }else {
        channel = RCSP_EAR_CHANNEL_RIGHT;
    }

    return get_adv_key_opt(key_action, channel);
}

static int key_set_setting_extra_handle(void *setting_data, void *setting_data_len)
{
    y_printf("%s() %d", __func__, __LINE__);
    //这里才是修改缓存列表的地方
    //修改之后会显示在APP界面上
    u8 dlen = *((u8 *)setting_data_len);
    u8 *key_setting_data = (u8 *)setting_data;
    while (dlen >= 3) {
        if (key_setting_data[0] == RCSP_EAR_CHANNEL_LEFT) {
            if (key_setting_data[1] == RCSP_KEY_ACTION_CLICK) {
                g_key_setting[2] = key_setting_data[2];
            } else if (key_setting_data[1] == RCSP_KEY_ACTION_DOUBLE_CLICK) {
                g_key_setting[8] = key_setting_data[2];
            } else if(key_setting_data[1] == RCSP_KEY_ACTION_LONG_CLICK){
                g_key_setting[14] = key_setting_data[2];
            } else if(key_setting_data[1] == RCSP_KEY_ACTION_THREE_CLICK){
                g_key_setting[20] = key_setting_data[2];
            }
        } else if (key_setting_data[0] == RCSP_EAR_CHANNEL_RIGHT) {
            if (key_setting_data[1] == RCSP_KEY_ACTION_CLICK) {
                g_key_setting[5] = key_setting_data[2];
            } else if (key_setting_data[1] == RCSP_KEY_ACTION_DOUBLE_CLICK) {
                g_key_setting[11] = key_setting_data[2];
            } else if(key_setting_data[1] == RCSP_KEY_ACTION_LONG_CLICK){
                g_key_setting[17] = key_setting_data[2];
            } else if(key_setting_data[1] == RCSP_KEY_ACTION_THREE_CLICK){
                g_key_setting[23] = key_setting_data[2];
            }
        }
        dlen -= 3;
        key_setting_data += 3;
    }
    y_printf("key_set_setting_extra_handle : 单击左:%d 单击右:%d 双击左:%d 双击右:%d 长按左:%d 长按右:%d 三击左:%d 三击右%d\n",
           g_key_setting[2], g_key_setting[5], g_key_setting[8], g_key_setting[11],
           g_key_setting[14], g_key_setting[17], g_key_setting[20], g_key_setting[23]);
    return 0;
}

static int key_get_setting_extra_handle(void *setting_data, void *setting_data_len)
{
    y_printf("%s() %d", __func__, __LINE__);
    int **setting_data_ptr = (int **)setting_data;
    *setting_data_ptr = (int *)g_key_setting;
    y_printf("key_get_setting_extra_handle : 单击左:%d 单击右:%d 双击左:%d 双击右:%d 长按左:%d 长按右:%d 三击左:%d 三击右%d\n",
           g_key_setting[2], g_key_setting[5], g_key_setting[8], g_key_setting[11],
           g_key_setting[14], g_key_setting[17], g_key_setting[20], g_key_setting[23]);
    return sizeof(g_key_setting);
}

static RCSP_SETTING_OPT adv_key_opt = {
    .data_len = 8,
    .setting_type = ATTR_TYPE_KEY_SETTING,
    .syscfg_id = CFG_RCSP_ADV_KEY_SETTING,
    .deal_opt_setting = deal_key_setting,
    .set_setting = set_key_setting,
    .get_setting = get_key_setting,
    .custom_setting_init = NULL,
    .custom_vm_info_update = NULL,
    .custom_setting_update = NULL,
    .custom_sibling_setting_deal = NULL,
    .custom_setting_release = NULL,
    .set_setting_extra_handle = key_set_setting_extra_handle,
    .get_setting_extra_handle = key_get_setting_extra_handle,
};
REGISTER_APP_SETTING_OPT(adv_key_opt);

#endif
#endif
