#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".in_ear_manage.data.bss")
#pragma data_seg(".in_ear_manage.data")
#pragma const_seg(".in_ear_manage.text.const")
#pragma code_seg(".in_ear_manage.text")
#endif
#include "in_ear_detect/in_ear_manage.h"
#include "in_ear_detect/in_ear_detect.h"
#include "system/includes.h"
#include "app_config.h"
#include "btstack/avctp_user.h"
#include "tone_player.h"
#include "app_msg.h"
#include "app_tone.h"

#include "event.h"

#define LOG_TAG_CONST       EAR_DETECT
#define LOG_TAG             "[EAR_DETECT]"
#define LOG_INFO_ENABLE
#include "debug.h"

//#define log_info(format, ...)       y_printf("[EAR_DETECT] : " format "\r\n", ## __VA_ARGS__)
#define log_info(format, ...)       printf("[EAR_DETECT] : " format "\r\n", ## __VA_ARGS__)


#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif

#if TCFG_EAR_DETECT_ENABLE

#if INEAR_ANC_UI
#include "audio_anc.h"
u8 inear_tws_ancmode = ANC_OFF;
// u8 inear_tws_ancmode = ANC_TRANSPARENCY;
#endif

struct ear_detect_d _ear_detect_d = {
    .toggle = 1,
    .music_en = 0, //上电开机默认不使能音乐控制
    .pre_music_sta = MUSIC_STATE_NULL,
    .pre_state = !TCFG_EAR_DETECT_DET_LEVEL, //默认没有入耳
    .cur_state = !TCFG_EAR_DETECT_DET_LEVEL, //默认没有入耳
    .tws_state = !TCFG_EAR_DETECT_DET_LEVEL, //默认没有入耳
    .bt_init_ok = 1,
    .music_check_timer = 0,
    .music_sta_cnt = 0,
    .change_master_timer = 0,
    .key_enable_timer = 0,
    .key_delay_able = 1,
    //TCFG_EAR_DETECT_MUSIC_CTL_EN
    .play_cnt = 0,
    .stop_cnt = 0,
    .music_ctl_timeout = 0,
    .a2dp_det_timer = 0,
    .music_regist_en = 0,
};

#define __this 	(&_ear_detect_d)

//入耳检测公共控制
u8 is_ear_detect_state_in(void)     //入耳返回1
{
    if (0 == __this->toggle) {
        return 1;
    }
    return (__this->cur_state == TCFG_EAR_DETECT_DET_LEVEL);
}

u8 is_ear_detect_tws_state_in(void) //对耳入耳返回1
{
    if (0 == __this->toggle) {
        return 1;
    }
    return (__this->tws_state == TCFG_EAR_DETECT_DET_LEVEL);
}

void ear_detect_music_ctl_delay_deal(void *priv)
{
    //暂停n秒内戴上控制音乐，超过后不能控制
    log_info("%s", __func__);
    __this->music_en = 0;
    __this->music_ctl_timeout = 0;
    __this->music_regist_en = 0;
}

void ear_detect_music_ctl_timer_del()
{
    log_info("%s", __func__);
    if (__this->music_ctl_timeout) {
        sys_timeout_del(__this->music_ctl_timeout);
        __this->music_ctl_timeout = 0;
    }
}

static void ear_detect_music_ctl_en(u8 en)
{
    log_info("%s, %d, music_en:%d, timeout:%d,regist_en:%d", __func__, en, __this->music_en, __this->music_ctl_timeout, __this->music_regist_en);
    if (en) {
        __this->music_en = en;
        if (__this->music_ctl_timeout) {
            log_info("del pause timeout deal");
            sys_timeout_del(__this->music_ctl_timeout);
            __this->music_ctl_timeout = 0;
        }
    } else {
        if (__this->music_regist_en == 0) {
            __this->music_en = 0;
            return;
        }
        if (__this->music_en && (0 == __this->music_ctl_timeout)) {
            log_info("regist pause timeout deal");
            if (__this->cfg->ear_det_music_ctl_ms) {
                __this->music_ctl_timeout = sys_timeout_add(NULL, ear_detect_music_ctl_delay_deal, __this->cfg->ear_det_music_ctl_ms);
            }
        }
    }
}

#define A2DP_PLAY_CNT           2
#define A2DP_STOP_CNT           2
static void ear_detect_a2dp_detech(void *priv)
{
    u8 a2dp_state = 0;
    if (bt_get_call_status() != BT_CALL_HANGUP) {
        return;
    }
    a2dp_state = bt_a2dp_get_status();
    //printf("a2dp: %d", a2dp_state);
    if (a2dp_state == BT_MUSIC_STATUS_STARTING) { //在播歌
        __this->stop_cnt = 0;
        if (__this->play_cnt < A2DP_PLAY_CNT) {
            __this->play_cnt++;
            if (__this->play_cnt == A2DP_PLAY_CNT) {
                __this->pre_music_sta = MUSIC_STATE_PLAY;
                ear_detect_music_ctl_en(1);
            }
        }
    } else { //不在播歌
        __this->play_cnt = 0;
        if (__this->stop_cnt < A2DP_STOP_CNT) {
            __this->stop_cnt++;
            if (__this->stop_cnt == A2DP_STOP_CNT) {
                if (__this->music_en) { //入耳检测已经使能
                    __this->pre_music_sta = MUSIC_STATE_PAUSE;
                    ear_detect_music_ctl_en(0);
                }
            }
        }
    }
}

void ear_detect_a2dp_det_en(u8 en)
{
    if (en) {
        __this->play_cnt = 0;
        __this->stop_cnt = 0;
        if (!__this->a2dp_det_timer) {
            log_info("ear_detect_a2dp_detech timer add");
            __this->a2dp_det_timer = sys_timer_add(NULL, ear_detect_a2dp_detech, 500);
        }
    } else {
        if (__this->a2dp_det_timer) {
            log_info("ear_detect_a2dp_detech timer del");
            sys_timer_del(__this->a2dp_det_timer);
            __this->a2dp_det_timer = 0;
        }
    }
}

static void cmd_post_key_msg(u8 user_msg)
{
    // struct sys_event e = {0};
    // e.type = SYS_KEY_EVENT;
    // e.u.key.event = KEY_EVENT_USER;
    // e.u.key.value = user_msg;

    // sys_event_notify(&e);
    printf("cmd_post_key_msg------------DEVICE_EVENT_FROM_POST_KEY:[%d]\r\n",user_msg);
    int msg[2];
    msg[0] = DEVICE_EVENT_FROM_POST_KEY;
    msg[1] = user_msg;

    os_taskq_post_type("app_core", MSG_FROM_IN_EAR, 2, msg);
}

// int cmd_key_msg_handle(struct sys_event *event)
static u8 old_msg = 0;
int cmd_key_msg_handle(int *msg)
{
    log_info("++++++++++++++++++%s ,%d\n", __func__,msg[1]);
    bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
    // struct key_event *key = &event->u.key;
    // switch (key->value) {
    // if (msg[0] != DEVICE_EVENT_FROM_POST_KEY)
    // {
    //     log_info("%s ,msg err\n", __func__);
    //     return 0;
    // }
    u8 addr[6];
    u8 *bt_addr = NULL;
    if ((old_msg == (u8)msg[1]) || ((msg[1] != 1) && (msg[1] != 2))) {
        return 0;
    }
    old_msg = (u8)msg[1];
    // if (a2dp_player_get_btaddr(addr)) {
    //     bt_addr = addr;
    // }
    
    switch (msg[1]) {
    case CMD_EAR_DETECT_MUSIC_PLAY:
        log_info("CMD_EAR_DETECT_MUSIC_PLAY");
        printf("CMD_EAR_DETECT_MUSIC_PLAY---phone_addr:");
        // put_buf(bt_addr,6);
        // bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
        printf("CMD_EAR_DETECT_MUSIC_PLAY---------bt_a2dp_get_status():[%d]\r\n",bt_a2dp_get_status());
        if (bt_a2dp_get_status() != BT_MUSIC_STATUS_STARTING) {
            printf("-------------------------------------------tws_api_get_role():[%d]-----------------\r\n",tws_api_get_role());
        // bt_cmd_prepare_for_addr(bt_addr, USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
            } else {
                tws_api_sync_call_by_uuid('T', SYNC_CMD_MUSIC_PLAY, 400);
            }
        }
        break;
    case CMD_EAR_DETECT_MUSIC_PAUSE:
        log_info("CMD_EAR_DETECT_MUSIC_PAUSE");
        printf("CMD_EAR_DETECT_MUSIC_PAUSE---phone_addr:");
        // put_buf(bt_addr,6);
        // bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PAUSE, 0, NULL);
        // bt_cmd_prepare_for_addr(bt_addr, USER_CTRL_AVCTP_OPID_PAUSE, 0, NULL);
        printf("CMD_EAR_DETECT_MUSIC_PAUSE---------bt_a2dp_get_status():[%d]\r\n",bt_a2dp_get_status());
        if (bt_a2dp_get_status() == BT_MUSIC_STATUS_STARTING) {
            printf("==========================================-tws_api_get_role():[%d]--===========\r\n",tws_api_get_role());
            // bt_cmd_prepare_for_addr(bt_addr, USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
            } else {
                tws_api_sync_call_by_uuid('T', SYNC_CMD_MUSIC_STOP, 400);
            }
        }
        break;
    // case CMD_EAR_DETECT_SCO_CONN:
    //     log_info("CMD_EDETECT_SCO_CONN");
    //     // bt_cmd_prepare(USER_CTRL_CONN_SCO, 0, NULL);
    //     bt_cmd_prepare_for_addr(bt_addr, USER_CTRL_CONN_SCO, 0, NULL);
    //     break;
    // case CMD_EAR_DETECT_SCO_DCONN:
    //     log_info("CMD_EAR_DETECT_SCO_DCONN");
    //     // bt_cmd_prepare(USER_CTRL_DISCONN_SCO, 0, NULL);
    //     bt_cmd_prepare_for_addr(bt_addr, USER_CTRL_DISCONN_SCO, 0, NULL);
    //     break;
    default:
        break;
    }
    return 0;
}

APP_MSG_PROB_HANDLER(in_ear_cmd_msg_entry) = {  
    .owner      = 0xff,
    .from       = MSG_FROM_IN_EAR,
    .handler    = cmd_key_msg_handle,
};

static void ear_detect_post_event(u8 event)
{
    // struct sys_event e;
    // bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
    // e.type = SYS_DEVICE_EVENT;
    // e.arg = (void *)DEVICE_EVENT_FROM_EAR_DETECT;
    // e.u.ear.value = event;
    // sys_event_notify(&e);
    int msg[2] = {0};
     printf("ear_detect_post_event------------DEVICE_EVENT_FROM_EAR_DETECT:[%d]\r\n",event);
    bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
    msg[0] = DEVICE_EVENT_FROM_EAR_DETECT;
    msg[1] = event;
    os_taskq_post_type("app_core", MSG_FROM_IN_EAR, 2, msg);
}

static void cancel_music_state_check()
{
    if (__this->music_check_timer) {
        sys_timer_del(__this->music_check_timer);
        __this->music_check_timer = 0;
        __this->music_sta_cnt = 0;
    }
}

static void music_play_state_check(void *priv)
{
    if (__this->pre_music_sta == MUSIC_STATE_PLAY) {
        if (__this->cfg->ear_det_in_music_sta == 1) {
            cancel_music_state_check();
        } else {
            if (bt_a2dp_get_status() == BT_MUSIC_STATUS_STARTING) {
                __this->music_sta_cnt++;
                log_info("play cnt: %d", __this->music_sta_cnt);
            } else { //期间变成了暂停
                printf("music_play_state_check---CMD_EAR_DETECT_MUSIC_PLAY\r\n");
                cmd_post_key_msg(CMD_EAR_DETECT_MUSIC_PLAY);
                cancel_music_state_check();
            }
            if (__this->music_sta_cnt >= __this->cfg->ear_det_music_play_cnt) {
                cancel_music_state_check();
            }
        }
    } else {
        if (bt_a2dp_get_status() != BT_MUSIC_STATUS_STARTING) {
            __this->music_sta_cnt++;
            log_info("pause cnt: %d", __this->music_sta_cnt);
        } else { //期间变成了暂停
            printf("music_play_state_check---CMD_EAR_DETECT_MUSIC_PAUSE\r\n");
            cmd_post_key_msg(CMD_EAR_DETECT_MUSIC_PAUSE);
            cancel_music_state_check();
        }
        if (__this->music_sta_cnt >= __this->cfg->ear_det_music_pause_cnt) {
            cancel_music_state_check();
        }
    }
}

static void ear_detect_music_play_ctl(u8 music_state)
{
    log_info("%s", __func__);
#if TCFG_USER_TWS_ENABLE
    if (get_tws_sibling_connect_state() && (__this->pre_music_sta == music_state))  //防止两只耳机陆续入耳，重复操作
#endif
    {
        log_info("tws same or notws . music_state: %d", music_state);
        if (__this->cfg->ear_det_music_ctl_en == 1) {
            return;
        }
    }
    __this->pre_music_sta = music_state;
    log_info("music state:%d music_en:%d", music_state, __this->music_en);
    if (__this->music_en) { //入耳检测控制音乐使能
        if (music_state == MUSIC_STATE_PLAY) { //播放音乐
            log_info("MUSIC_STATE_PLAY");
            if (__this->cfg->ear_det_music_ctl_en == 1) {
                __this->music_regist_en = 0;
            }
            if (__this->cfg->ear_det_in_music_sta == 0) {
                if (bt_a2dp_get_status() != BT_MUSIC_STATUS_STARTING) { //没有播放
                    log_info("-------1");
                    cmd_post_key_msg(CMD_EAR_DETECT_MUSIC_PLAY);
                } else {
                    log_info("-------2");
                    cancel_music_state_check();
                    __this->music_check_timer = sys_timer_add(NULL, music_play_state_check, 100);
                }
            }
        } else { //暂停音乐
            log_info("MUSIC_STATE_PAUSE");
            if (__this->cfg->ear_det_music_ctl_en == 1) {
                __this->music_regist_en = 1;
            }
            if (bt_a2dp_get_status() == BT_MUSIC_STATUS_STARTING) { //正在播放
                log_info("-------3");
                cmd_post_key_msg(CMD_EAR_DETECT_MUSIC_PAUSE);
            } else {
                log_info("-------4");
                cancel_music_state_check();
                __this->music_check_timer = sys_timer_add(NULL, music_play_state_check, 100);
            }
        }
    }
}

static u8 ear_detect_check_online(u8 check_mode)
{
    if (check_mode == 1) {
#if TCFG_USER_TWS_ENABLE
        if ((get_tws_sibling_connect_state() && (__this->cur_state != TCFG_EAR_DETECT_DET_LEVEL) && (__this->tws_state != TCFG_EAR_DETECT_DET_LEVEL)) //对耳且都不在耳朵上
            || (!get_tws_sibling_connect_state() && (__this->cur_state != TCFG_EAR_DETECT_DET_LEVEL)))  //单耳且不在耳朵
#else
        if (__this->cur_state != TCFG_EAR_DETECT_DET_LEVEL)
#endif
        {
            return 1;
        }
        return 0;
    } else if (check_mode == 0) {
#if TCFG_USER_TWS_ENABLE
        if ((get_tws_sibling_connect_state() && ((__this->cur_state == TCFG_EAR_DETECT_DET_LEVEL) || (__this->tws_state == TCFG_EAR_DETECT_DET_LEVEL))) //对耳戴上一只
            || (!get_tws_sibling_connect_state() && (__this->cur_state == TCFG_EAR_DETECT_DET_LEVEL)))  //单耳且入耳
#else
        if (__this->cur_state == TCFG_EAR_DETECT_DET_LEVEL)
#endif
        {
            return 1;
        }
        return 0;
    }
    return 0;
}

void ear_detect_phone_active_deal()
{
    if (0 == __this->toggle) {
        return;
    }
    if (ear_detect_check_online(1) == 1) {
        cmd_post_key_msg(CMD_EAR_DETECT_SCO_DCONN);
    }
}

#if (TCFG_EAR_DETECT_AUTO_CHG_MASTER && TCFG_USER_TWS_ENABLE)
// extern void tws_conn_switch_role();
// extern void test_esco_role_switch(u8 flag);
extern void tws_api_role_switch();
void ear_detect_change_master_timeout_deal(void *priv) //主从切换
{
    y_printf("%s", __func__);
    __this->change_master_timer = 0;
    tws_api_auto_role_switch_disable();
    // test_esco_role_switch(1); //主机调用
    tws_api_role_switch();
}

void ear_detect_change_master_timer_del()
{
    if (__this->change_master_timer) {
        y_printf("%s", __func__);
        sys_timeout_del(__this->change_master_timer);
        __this->change_master_timer = 0;
    }
    tws_api_auto_role_switch_enable(); //恢复主从自动切换
}

//开始通话时，判断主机是否在耳朵上，不在切换主从
void ear_detect_call_chg_master_deal()
{
    y_printf("self:%d tws:%d\n", __this->cur_state, __this->tws_state);
    if (0 == __this->toggle) {
        return;
    }
    if (get_tws_sibling_connect_state() && (tws_api_get_role() == TWS_ROLE_MASTER) && (__this->cur_state != TCFG_EAR_DETECT_DET_LEVEL) && (__this->tws_state == TCFG_EAR_DETECT_DET_LEVEL)) { //主机不在耳，从机在耳，切换主从
        y_printf("%s", __func__);
        ear_detect_change_master_timeout_deal(NULL);
    }
}
#endif

static void ear_detect_in_deal()   //主机才会执行
{
    log_info("%s", __func__);
    u8 call_status = bt_get_call_status();
    y_printf("[EAR_DETECT] : in self:%d tws:%d call_st:%d\n", __this->cur_state, __this->tws_state, call_status);
    if (0 == __this->toggle) {
        return;
    }
    if (call_status != BT_CALL_HANGUP) {//通话中
#if TCFG_EAR_DETECT_CALL_CTL_SCO
        if (call_status == BT_CALL_ACTIVE) {//通话中
            if (ear_detect_check_online(0) == 1) {
                log_info("CMD_CTRL_CONN_SCO\n");
                cmd_post_key_msg(CMD_EAR_DETECT_SCO_CONN);
            }
        }
#endif
#if (TCFG_EAR_DETECT_AUTO_CHG_MASTER && TCFG_USER_TWS_ENABLE)
        ear_detect_change_master_timer_del();
        if (get_tws_sibling_connect_state() && (__this->cur_state != TCFG_EAR_DETECT_DET_LEVEL) && (__this->tws_state == TCFG_EAR_DETECT_DET_LEVEL)) { //主机不在耳，从机在耳，切换主从
            y_printf("master no inside, start change role");
            __this->change_master_timer = sys_timeout_add(NULL, ear_detect_change_master_timeout_deal, 2000);
        }
#endif
    } else if (bt_get_total_connect_dev()) { //已连接
        if ((__this->cur_state == TCFG_EAR_DETECT_DET_LEVEL) || (__this->tws_state == TCFG_EAR_DETECT_DET_LEVEL)) { //可以自定义控制暂停播放的条件
            printf("ear_detect_in_deal---------------------\r\n");
            ear_detect_music_play_ctl(MUSIC_STATE_PLAY);
        }
    }
}

static void ear_detect_out_deal()   //主机才会执行
{
    log_info("%s", __func__);
    u8 call_status = bt_get_call_status();
    y_printf("[EAR_DETECT] : out self:%d tws:%d call_st:%d\n", __this->cur_state, __this->tws_state, call_status);
    if (0 == __this->toggle) {
        return;
    }
    if (call_status != BT_CALL_HANGUP) {//通话中
        if (call_status == BT_CALL_ACTIVE) {//通话中
            if (ear_detect_check_online(1) == 1) {
#if (TCFG_EAR_DETECT_AUTO_CHG_MASTER && TCFG_USER_TWS_ENABLE)
                ear_detect_change_master_timer_del(); //通话中如果主机摘掉，然后从机也摘掉，不切换主从
#endif
#if TCFG_EAR_DETECT_CALL_CTL_SCO
                cmd_post_key_msg(CMD_EAR_DETECT_SCO_DCONN);
                log_info("CMD_CTRL_DISCONN_SCO\n");
#endif
            }
#if (TCFG_EAR_DETECT_AUTO_CHG_MASTER && TCFG_USER_TWS_ENABLE)
            ear_detect_change_master_timer_del();
            if (get_tws_sibling_connect_state() && (__this->cur_state != TCFG_EAR_DETECT_DET_LEVEL) && (__this->tws_state == TCFG_EAR_DETECT_DET_LEVEL)) { //主机不在耳，从机在耳，切换主从
                y_printf("master no inside, start change role");
                __this->change_master_timer = sys_timeout_add(NULL, ear_detect_change_master_timeout_deal, 2000);
            }
#endif
        }
    } else if (bt_get_total_connect_dev()) { //已连接
        if ((__this->cur_state != TCFG_EAR_DETECT_DET_LEVEL) || (__this->tws_state != TCFG_EAR_DETECT_DET_LEVEL)) { //可以自定义控制暂停播放的条件
            ear_detect_music_play_ctl(MUSIC_STATE_PAUSE);
        }
    }
}

#if TCFG_USER_TWS_ENABLE
void tws_sync_ear_detect_state(u8 need_do)
{
    u8 state = 0;
    state = need_do ? (BIT(7) | __this->cur_state) : __this->cur_state;
    if (!need_do) {
        state |= (__this->music_en << 6);
    }
    r_printf("[EAR_DETECT] : %s: %x , %x ", __func__, state, __this->cur_state);
    tws_api_send_data_to_sibling(&state, 1, TWS_FUNC_ID_EAR_DETECT_SYNC);
}

static void tws_sync_ear_detect_state_deal(void *_data, u16 len, bool rx) //在这个回调里面，不能执行太久，不要调用tws_api_get_role()和bt_cmd_prepare，会偶尔死机
{
    u8 *data = (u8 *)_data;
    u8 state = data[0];

    if (rx) {
        r_printf("[EAR_DETECT] : %s: %x , rx = %x", __func__, state, rx);
        __this->tws_state = state & BIT(0);
        if ((state & BIT(7))) { //主机并且需要执行 // && (tws_api_get_role() == TWS_ROLE_MASTER)
            if (__this->tws_state == TCFG_EAR_DETECT_DET_LEVEL) {
                ear_detect_post_event(EAR_DETECT_EVENT_IN_DEAL);
            } else {
                ear_detect_post_event(EAR_DETECT_EVENT_OUT_DEAL);
            }
        }
    }
}
REGISTER_TWS_FUNC_STUB(ear_detect_sync) = {
    .func_id = TWS_FUNC_ID_EAR_DETECT_SYNC,
    .func    = tws_sync_ear_detect_state_deal,
};
#endif

void ear_detect_change_state_to_event(u8 state)
{
    u8 event = EAR_DETECT_EVENT_NULL;
    log_info(" __this->pre_state===%d",  __this->pre_state);
    log_info(" __this->cur_state===%d",  __this->cur_state);
    __this->pre_state = __this->cur_state;
    __this->cur_state = state;
    if (__this->cur_state == __this->pre_state) {
        log_info("same state,return");
        return;
    }
    log_info("post event, cur_state:%d, pre_state:%d", __this->cur_state, __this->pre_state);
    event = (state == TCFG_EAR_DETECT_DET_LEVEL) ? EAR_DETECT_EVENT_IN : EAR_DETECT_EVENT_OUT;
    ear_detect_post_event(event);
}

static void __ear_detect_in_dealy_deal(void *priv)
{
    static u16 tone_delay_in_deal = 0;
    // if (!tone_get_status()) {
    extern int tone_player_runing();
    if(!tone_player_runing()){
        sys_timer_del(tone_delay_in_deal);
        tone_delay_in_deal = 0;
        ear_detect_in_deal();
    } else {
        if (!tone_delay_in_deal) {
            tone_delay_in_deal = sys_timer_add(NULL, __ear_detect_in_dealy_deal, 100);
        }
    }
}

static void ear_detect_set_key_delay_able(void *priv)
{
    u8 able = (u8)priv;
    __this->key_delay_able = able;
}

u8 ear_detect_get_key_delay_able(void)
{
    return __this->key_delay_able;
}
#if INEAR_ANC_UI
void etch_in_anc(void)
{
    if (get_tws_sibling_connect_state()) {//tws
        printf("etch_in_anc---------__this->tws_state:[%d]-__this->cur_state:[%d]-inear_tws_ancmode:[%d]",__this->tws_state,__this->cur_state,inear_tws_ancmode);
        if (__this->tws_state && __this->cur_state) {
            if (inear_tws_ancmode == ANC_ON) {
                log_info("switch anc mode\n");
                // anc_mode_switch(inear_tws_ancmode, 0);
                anc_mode_switch(inear_tws_ancmode, 1);
                inear_tws_ancmode = 1;
                // inear_tws_ancmode = 3;
            }
        } else if (__this->cur_state && !__this->tws_state) { //单耳入耳
            if (inear_tws_ancmode == ANC_ON) {
                // anc_mode_switch(ANC_TRANSPARENCY, 0);
                anc_mode_switch(ANC_OFF, 0);
            }
        }
    } else {
        printf("etch_in_anc----anc_mode_get:[%d]--\r\n",anc_mode_get());
    }
}

void etch_out_anc(void)
{
    if (get_tws_sibling_connect_state()) {//tws
        printf("etch_out_anc---------anc_mode_get():[%d]--__this->cur_state:[%d]-inear_tws_ancmode:[%d]",anc_mode_get(),__this->cur_state,inear_tws_ancmode);
        // if (anc_mode_get() == ANC_ON) {
            inear_tws_ancmode = ANC_ON;
            // if (!__this->cur_state) { //还在耳朵上的那只耳机
            //     anc_mode_switch(ANC_OFF, 0);
            // } else {
                // anc_mode_switch(ANC_TRANSPARENCY, 0);
                anc_mode_switch(ANC_OFF, 0);

            // }
        // } else if (anc_mode_get() == ANC_TRANSPARENCY) {
        //     // if (inear_tws_ancmode == ANC_ON) {
        //     //     anc_mode_switch(ANC_OFF, 0);
        //     // }
        // } else if (anc_mode_get() == ANC_OFF) {
        //     anc_mode_switch(ANC_TRANSPARENCY, 0);
        // }
    } else {
        printf("etch_out_anc----anc_mode_get:[%d]--\r\n",anc_mode_get());
    }
}
#endif
void tone_play_deal(const char *name, u8 preemption, u8 add_en);
// void ear_detect_event_handle(u8 state)
int ear_detect_event_handle(int *msg)
{  log_info("%s", __func__);
    // switch (state) {
    log_info("%s ,%d\n", __func__,msg[1]);
    if (msg[0] != DEVICE_EVENT_FROM_EAR_DETECT)
    {
        log_info("%s ,msg err\n", __func__);
        return 0;
    }
    
    switch (msg[1]) {
    case EAR_DETECT_EVENT_NULL:
        log_info("EAR_DETECT_EVENT_NULL");
        break;
    case EAR_DETECT_EVENT_IN:
        log_info("EAR_DETECT_EVENT_IN");
#if INEAR_ANC_UI
        etch_in_anc();
#endif
        log_info("toggle = %d,call_status = %d\n", __this->toggle, bt_get_call_status());
        if (__this->toggle && __this->bt_init_ok && (bt_get_call_status() == BT_CALL_HANGUP)) {
#if TCFG_USER_TWS_ENABLE
            if (get_tws_sibling_connect_state()) { //对耳链接上了，对耳不在耳时播，第一只播
                if (__this->tws_state != TCFG_EAR_DETECT_DET_LEVEL) { //对耳已经戴上了，不播放
                    // bt_tws_play_tone_at_same_time(SYNC_TONE_EARDET_IN, 400);
                    // tws_play_tone_file(get_tone_files()->ear_check, 400);
                }
            } else //对耳没连接上，自己决定

#endif
            {
                // tone_play(TONE_EAR_CHECK, 1);
                // play_tone_file(get_tone_files()->ear_check);
            }
        }
#if TCFG_USER_TWS_ENABLE
        if (get_tws_sibling_connect_state()) {
            tws_sync_ear_detect_state(1);
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                __ear_detect_in_dealy_deal(NULL);
            }
        } else
#endif
        {
            __ear_detect_in_dealy_deal(NULL);
        }
        if ((__this->key_enable_timer == 0) && (__this->cfg->ear_det_key_delay_time != 0)) {
            __this->key_enable_timer = sys_timeout_add((void *)1, ear_detect_set_key_delay_able, __this->cfg->ear_det_key_delay_time);
        }
        break;
    case EAR_DETECT_EVENT_OUT:
        log_info("EAR_DETECT_EVENT_OUT");
#if INEAR_ANC_UI
        etch_out_anc();
#endif
#if TCFG_USER_TWS_ENABLE
        if (get_tws_sibling_connect_state()) {
            tws_sync_ear_detect_state(1);
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                ear_detect_out_deal();
            }
        } else
#endif
        {
            ear_detect_out_deal();
        }
        if (__this->cfg->ear_det_key_delay_time != 0) {
            ear_detect_set_key_delay_able(0);
            if (__this->key_enable_timer) {
                sys_timeout_del(__this->key_enable_timer);
                __this->key_enable_timer = 0;
            }
        }
        break;
#if TCFG_USER_TWS_ENABLE
    case EAR_DETECT_EVENT_IN_DEAL:
#if INEAR_ANC_UI
        etch_in_anc();
#endif
        log_info("EAR_DETECT_EVENT_IN_DEAL");
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            __ear_detect_in_dealy_deal(NULL);
        }
        break;
    case EAR_DETECT_EVENT_OUT_DEAL:
        log_info("EAR_DETECT_EVENT_OUT_DEAL");
#if INEAR_ANC_UI
        etch_out_anc();
#endif
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            ear_detect_out_deal();
        }
        break;
#endif
    default:
        break;
    }
}

APP_MSG_PROB_HANDLER(ear_detect_msg_entry) = {  
    .owner      = 0xff,
    .from       = MSG_FROM_IN_EAR,
    .handler    = ear_detect_event_handle,
};

void ear_touch_edge_wakeup_handle(u8 index, u8 gpio)
// void ear_touch_edge_wakeup_handle(void)
{  log_info("%s", __func__);
    u8 io_state = 0;
    ASSERT(gpio == TCFG_EAR_DETECT_DET_IO);

    io_state = gpio_read(TCFG_EAR_DETECT_DET_IO);
    printf("gpio_read(TCFG_EAR_DETECT_DET_IO)==%d",gpio_read(TCFG_EAR_DETECT_DET_IO));
    if (io_state == TCFG_EAR_DETECT_DET_LEVEL) {
        log_info("earphone touch in\n");
#if TCFG_KEY_IN_EAR_FILTER_ENABLE
        extern u8 io_key_filter_flag;
        if (gpio_read(IO_PORTB_01) == 0) {
            io_key_filter_flag = 1;
        } else {
            io_key_filter_flag = 0;
        }
#endif
        ear_detect_change_state_to_event(TCFG_EAR_DETECT_DET_LEVEL);
    } else {
        log_info("earphone touch out\n");
        ear_detect_change_state_to_event(!TCFG_EAR_DETECT_DET_LEVEL);
    }

    if (io_state) {
        //current: High
        
        // power_wakeup_set_edge(TCFG_EAR_DETECT_DET_IO, FALLING_EDGE);
        p33_io_wakeup_edge(TCFG_EAR_DETECT_DET_IO, FALLING_EDGE);
    } else {
        //current: Low
        // power_wakeup_set_edge(TCFG_EAR_DETECT_DET_IO, RISING_EDGE);
        p33_io_wakeup_edge(TCFG_EAR_DETECT_DET_IO, RISING_EDGE);
    }
}

// static void ear_det_app_event_handler(struct sys_event *event)
static void ear_det_app_event_handler(int *_event)
{
    log_info("%s", __func__);
    // struct bt_event *bt = &(event->u.bt);
    if(!__this->cfg->ear_det_music_ctl_en){
        return;
    }
    struct bt_event *event = (struct bt_event *)_event;
    // switch (event->type) {
    log_info("event->event==%d", event->event);
    switch (event->event) {
    // case SYS_BT_EVENT:
    //     if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            // switch (bt->event) {
            case BT_STATUS_FIRST_CONNECTED:
            case BT_STATUS_SECOND_CONNECTED:
                log_info("BT_STATUS_CONNECTED\n");
                if (__this->cfg->ear_det_music_ctl_en) {
                    ear_detect_a2dp_det_en(1);
                }
                break;
            case BT_STATUS_FIRST_DISCONNECT:
            case BT_STATUS_SECOND_DISCONNECT:
                log_info("BT_STATUS_DISCONNECT\n");
                if (__this->cfg->ear_det_music_ctl_en) {
                    ear_detect_a2dp_det_en(0);
                }
                break;
            case BT_STATUS_PHONE_ACTIVE:
                log_info("BT_STATUS_PHONE_ACTIVE\n");
                #if TCFG_EAR_DETECT_ENABLE
                    #if TCFG_EAR_DETECT_CALL_CTL_SCO
                            ear_detect_phone_active_deal();
                    #endif
                    #if (TCFG_EAR_DETECT_AUTO_CHG_MASTER && TCFG_USER_TWS_ENABLE)
                            ear_detect_call_chg_master_deal();
                    #endif //TCFG_EAR_DETECT_AUTO_CHG_MASTER
                #endif // TCFG_EAR_DETECT_ENABLE
                break;
            case BT_STATUS_PHONE_HANGUP:
                log_info("BT_STATUS_PHONE_HANGUP\n");
                #if (TCFG_EAR_DETECT_ENABLE && TCFG_EAR_DETECT_AUTO_CHG_MASTER && TCFG_USER_TWS_ENABLE)
                    ear_detect_change_master_timer_del(); //假如通话结束了，但是还没开始切换主从，取消切换
                #endif
                break;
            default:
                break;
            // }
        // }
        // break;
    // default:
    //     break;
    }
}

APP_MSG_HANDLER(ear_det_app_msg_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = ear_det_app_event_handler,
};

/************************** EAR DETECT  ****************************/
#if TCFG_EAR_DETECT_ENABLE
const struct ear_detect_platform_data ear_detect_cfg = {
	.ear_det_music_ctl_en = 1,			     	 //音乐控制使能
	// .ear_det_music_ctl_ms = 15000,    	         //音乐暂停之后，入耳检测控制暂停播放的时间
	.ear_det_music_ctl_ms = 0,    	         //音乐暂停之后，入耳检测控制暂停播放的时间
	.ear_det_in_music_sta = 0,                   // 0：入耳播歌  1：入耳不播歌
	.ear_det_key_delay_time = 0,    	         //入耳后按键起效时间ms ( 0 : OFF )
	//timer检测
	.ear_det_in_cnt = 3,          	    	 //戴上消抖
	.ear_det_out_cnt = 2,			    	 //拿下消抖
	// .ear_det_in_cnt = 4,          	    	 //戴上消抖
	// .ear_det_out_cnt = 5,			    	 //拿下消抖
	//IR
	.ear_det_ir_enable_time = 2,          		 //使能时长
	.ear_det_ir_disable_time = 300,				 //休眠时长
	.ear_det_ir_compensation_en = 0,				 //防太阳光干扰
	//音乐检测
	.ear_det_music_play_cnt = 20,          		 //音乐播放检测时间
	.ear_det_music_pause_cnt = 10,				 //音乐暂停检测时间
};
#endif //TCFG_EAR_DETECT_ENABLE

//配置边沿唤醒的IO, 当边沿唤醒事件发生时回调
static void in_ear_port_edge_wakeup_callback( u8 index, u8 gpio)
{
#if TCFG_EAR_DETECT_ENABLE
#if (TCFG_EAR_DETECT_TYPE == EAR_DETECT_BY_TOUCH) && (!TCFG_EAR_DETECT_TOUCH_MODE)
	if (gpio == TCFG_EAR_DETECT_DET_IO) {
		// ear_touch_edge_wakeup_handle(gpio);
        ear_touch_edge_wakeup_handle(index,gpio);
        // ear_touch_edge_wakeup_handle();
	}
#endif
#endif /* #if TCFG_EAR_TCH_ENABLE */
}

void ear_detect_init(void)
{
    printf("ear_detect_init");
    log_info("%s", __func__);
    ASSERT(&ear_detect_cfg);
    __this->cfg = &ear_detect_cfg;

#if TCFG_EAR_DETECT_ENABLE
#if (TCFG_EAR_DETECT_TYPE == EAR_DETECT_BY_TOUCH) && (!TCFG_EAR_DETECT_TOUCH_MODE)
	void ear_detect_tch_wakeup_init(void);
	ear_detect_tch_wakeup_init();
	// p33_io_wakeup_set_callback(TCFG_EAR_DETECT_DET_IO,in_ear_port_edge_wakeup_callback);
    // in_ear_port_edge_wakeup_callback();
    in_ear_port_edge_wakeup_callback(0,TCFG_EAR_DETECT_DET_IO);
#endif
#endif

#if (TCFG_EAR_DETECT_TYPE == EAR_DETECT_BY_IR)
    ear_detect_ir_init(&ear_detect_cfg);
#elif (TCFG_EAR_DETECT_TYPE == EAR_DETECT_BY_TOUCH)
    ear_detect_tch_init(&ear_detect_cfg);
#endif

}
__initcall(ear_detect_init);

#endif

