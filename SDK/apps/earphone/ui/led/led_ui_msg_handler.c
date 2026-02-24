#include "app_msg.h"
#include "app_main.h"
#include "bt_tws.h"
#include "asm/charge.h"
#include "battery_manager.h"
#include "pwm_led/led_ui_api.h"
#include "a2dp_player.h"
#include "esco_player.h"
#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
#include "cig.h"
#include "app_le_connected.h"
#endif


#define LOG_TAG             "[LED_UI]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#include "customer.h"


#if (TCFG_PWMLED_ENABLE)

u8 app_msg_power_on_flag = 0;//开机灯效标志
u8 app_msg_bt_in_page_mode_flag=0;//超距回连标志

static int ui_battery_msg_handler(int *msg)
{
    switch (msg[0]) {
    case BAT_MSG_CHARGE_START:
        led_ui_set_state(_LED_BAT_MSG_CHARGE_START_NAME, _LED_BAT_MSG_CHARGE_START_DISP_MODE);
        log_info("MSG_FROM_BATTERY----ui_battery_msg_handler----BAT_MSG_CHARGE_START");
        break;
    case BAT_MSG_CHARGE_FULL:
        //没有开启灯效保护，之前一直在这里配置
        //led_ui_set_state(_LED_BAT_MSG_CHARGE_FULL_NAME, _LED_BAT_MSG_CHARGE_FULL_DISP_MODE);
        log_info("MSG_FROM_BATTERY----ui_battery_msg_handler----BAT_MSG_CHARGE_FULL");
        //break;
    case BAT_MSG_CHARGE_CLOSE:
        //开启灯口保护，充满后灯效最后走的这里，不知道原因
        led_ui_set_state(_LED_BAT_MSG_CHARGE_CLOSE_NAME, _LED_BAT_MSG_CHARGE_CLOSE_DISP_MODE);
        log_info("MSG_FROM_BATTERY----ui_battery_msg_handler----BAT_MSG_CHARGE_CLOSE");
        break;
    case BAT_MSG_CHARGE_ERR:
        //没看见进来过，暂时熄灯
        log_info("MSG_FROM_BATTERY----ui_battery_msg_handler----BAT_MSG_CHARGE_ERR");
        //led_ui_set_state(LED_STA_ALL_OFF, DISP_CLEAR_OTHERS | DISP_TWS_SYNC);
        //break;
    case BAT_MSG_CHARGE_LDO5V_OFF:
        //没有看见进来过，暂时熄灯
        //led_ui_set_state(_LED_BAT_MSG_CHARGE_OTHER_NAME, _LED_BAT_MSG_CHARGE_OTHER_DISP_MODE);
        log_info("MSG_FROM_BATTERY----ui_battery_msg_handler----BAT_MSG_CHARGE_LDO5V_OFF");
        led_ui_set_state(LED_STA_ALL_OFF, DISP_CLEAR_OTHERS | DISP_TWS_SYNC);
        break;
    case POWER_EVENT_POWER_WARNING:
        log_info("MSG_FROM_BATTERY----ui_battery_msg_handler----POWER_EVENT_POWER_WARNING");
    case POWER_EVENT_POWER_LOW:
        //低电灯效
        led_ui_set_state(_LED_POWER_EVENT_POWER_LOW_NAME, _LED_POWER_EVENT_POWER_LOW_DISP_MODE);
        log_info("MSG_FROM_BATTERY----ui_battery_msg_handler----POWER_EVENT_POWER_LOW");
        break;
    }
    return 0;
}

static void led_enter_mode(u8 mode)
{
    switch (mode) {
    case APP_MODE_POWERON:
        break;
    case APP_MODE_BT:
        //这个作为开机灯效后的未连接灯效，与开机灯效理论上是前后衔接为一体的        
        led_ui_set_state(_LED_APP_MSG_POWER_ON_AFTER_NAME, _LED_APP_MSG_POWER_ON_AFTER_DISP_MODE);
        log_info("MSG_FROM_APP----led_enter_mode----APP_MSG_ENTER_MODE----APP_MODE_BT");
        
        break;
    case APP_MODE_PC:
    case APP_MODE_LINEIN:
        //led_ui_set_state(LED_STA_ALL_OFF, DISP_TWS_SYNC);
        break;

    }
}

static int ui_app_msg_handler(int *msg)
{
    char channel = tws_api_get_local_channel();
    int tws_state = tws_api_get_tws_state();
    
    switch (msg[0]) {
    case APP_MSG_ENTER_MODE:
        //手动开机是从APP_MSG_POWER_ON中途经过APP_MSG_ENTER_MODE
        //但是出仓开机的话，似乎不会走APP_MSG_POWER_ON，如果不设置灯效的话，出仓灯效直接丢失
        led_enter_mode(msg[1] & 0xff);
        app_msg_power_on_flag = 1;
        break;
    case APP_MSG_POWER_ON:
        // 开机灯效选择BT_STATUS_INIT_OK兼容手动开机以及出仓开机的灯效
        
        //led_ui_set_state(_LED_APP_MSG_POWER_ON_NAME, _LED_APP_MSG_POWER_ON_DISP_MODE); 

        log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_POWER_ON");
        break;
    //公版没有
    // case APP_MSG_KEY_POWER_OFF_HOLD:
    // case APP_MSG_KEY_POWER_OFF_RELEASE:
    // case APP_MSG_KEY_POWER_OFF_INSTANTLY:
    // case APP_MSG_KEY_POWER_OFF:
    // case APP_MSG_SOFT_POWEROFF:
    // case APP_MSG_TWS_POWER_OFF://新增会导致反复进入导致灯效不正常。
    //apps\earphone\mode\bt\tws_poweroff.c的sys_enter_soft_poweroff有注释
    //所有关机消息中都会发送一个APP_MSG_POWER_OFF消息用来更新灯效。
    case APP_MSG_POWER_OFF:
        // 超时自动关机
        if (msg[1] == POWEROFF_NORMAL ||
            msg[1] == POWEROFF_NORMAL_TWS) {
            led_ui_set_state(_LED_APP_MSG_POWER_OFF_NAME, _LED_APP_MSG_POWER_OFF_DISP_MODE);
        }
        log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_POWER_OFF");
        break;
    case APP_MSG_TWS_PAIRED:
        log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_TWS_PAIRED");
    case APP_MSG_TWS_UNPAIRED:
        //这里也不设置，避免打断开机灯效
        log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_TWS_UNPAIRED");
        break;
    case APP_MSG_BT_IN_PAGE_MODE:
        // 超距回连状态
        if(app_msg_power_on_flag == 1){
            //耳机有配对记录时或者手机有记录，开机会走这里，这里不设置灯效
            log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_BT_IN_PAGE_MODE----app_msg_power_on_flag");
            //当真的超距回连状态来时app_msg_power_on_flag应该为0
            //什么时候置0？当连接手机蓝牙后，这个置0，那么超距断开时回连就会走到下面。
        } else {
#if _LED_POWER_LOW_NO_INTERRUPT_ENABLE
            if (app_var.power_low_no_interrupt_flag){
                //超距灯效不能打断低电灯效
                log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_BT_IN_PAGE_MODE----power_low_no_interrupt_flag");
                break;
            }
#endif
            if(tws_state & TWS_STA_PHONE_CONNECTED){
                //为啥连接蓝牙后会进入这里，需要注释看打印排查，暂时这样
                log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_BT_IN_PAGE_MODE----TWS_STA_PHONE_CONNECTED----break");
                break;
            }
            
            if(app_msg_bt_in_page_mode_flag == 1){
                //已经进入过超距断开灯效，不需要重复设置
                log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_BT_IN_PAGE_MODE----app_msg_bt_in_page_mode_flag");
                break;
            }
            //超距断开灯效
            //这里因为会反复进入，导致一些周期灯效会被混乱，需要加一个判断只需要刷新一次即可
            //蓝牙连接后重新清除标志
            app_msg_bt_in_page_mode_flag = 1;

#if _LED_APP_MSG_BT_IN_PAGE_MODE_LEFT_RIGHT
        //灯效分左右
        if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
            (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
            led_ui_set_state(_LED_APP_MSG_BT_IN_PAGE_MODE_LEFT_NAME, _LED_APP_MSG_BT_IN_PAGE_MODE_LEFT_DISP_MODE);  
        }else {
            led_ui_set_state(_LED_APP_MSG_BT_IN_PAGE_MODE_RIGHT_NAME, _LED_APP_MSG_BT_IN_PAGE_MODE_RIGHT_DISP_MODE);    
        }
#elif _LED_APP_MSG_BT_IN_PAGE_MODE_MASTER_SLAVE
        //灯效分主从
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            led_ui_set_state(_LED_APP_MSG_BT_IN_PAGE_MODE_MASTER_NAME, _LED_APP_MSG_BT_IN_PAGE_MODE_MASTER_DISP_MODE);
        }else if(tws_api_get_role() == TWS_ROLE_SLAVE){
            led_ui_set_state(_LED_APP_MSG_BT_IN_PAGE_MODE_SLAVE_NAME, _LED_APP_MSG_BT_IN_PAGE_MODE_SLAVE_DISP_MODE);
        }
#else
        //单耳时灯效
        if(tws_state & TWS_STA_SIBLING_DISCONNECTED){
            //单耳状态闪烁谁的灯效
            led_ui_set_state(_LED_APP_MSG_BT_IN_PAGE_MODE_TWS_STA_SIBLING_DISCONNECTED_NAME, _LED_APP_MSG_BT_IN_PAGE_MODE_TWS_STA_SIBLING_DISCONNECTED_DISP_MODE);   
        }else {
            //双耳同步灯效
            led_ui_set_state(_LED_APP_MSG_BT_IN_PAGE_MODE_NAME, _LED_APP_MSG_BT_IN_PAGE_MODE_DISP_MODE);
        }
#endif
        log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_BT_IN_PAGE_MODE");
        }        
        break;
    case APP_MSG_BT_IN_PAIRING_MODE:
        // 等待手机连接状态----这里继承蓝牙断开灯效，不然会打乱超距回连灯效。
        //回连超时后进入这里，不打断超距回连灯效，不打断单耳蓝牙断开灯效
        log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_BT_IN_PAIRING_MODE");
#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
        if ((bt_get_total_connect_dev() == 0) && !is_cig_phone_conn() && !is_cig_other_phone_conn() && !app_var.goto_poweroff_flag) {
#else
        if ((bt_get_total_connect_dev() == 0) && !app_var.goto_poweroff_flag) {
#endif
            //led_ui_set_state(LED_STA_BLUE_FLASH_1TIMES_PER_1S, DISP_TWS_SYNC);
        }
        break;
    }
    return 0;
}

#if TCFG_BT_DUAL_CONN_ENABLE && _TCFG_BT_DUAL_CONN_ALL_DISCONNECT_ENABLE
static u8 bt_total_connect_num = 0;
#endif
static int ui_bt_stack_msg_handler(int *msg)
{
    struct bt_event *bt = (struct bt_event *)msg;
    char channel = tws_api_get_local_channel();
    int tws_state = tws_api_get_tws_state();
//这里不注释，下面主从灯效不生效    
// #if TCFG_USER_TWS_ENABLE
//     int tws_role = tws_api_get_role_async();
//     if (tws_role == TWS_ROLE_SLAVE) {
//         return 0;
//     }
// #endif

    printf("ui_bt_stack_msg_handler:%d\n", bt->event);
    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        //开机灯效选择BT_STATUS_INIT_OK兼容手动开机以及出仓开机的灯效
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_INIT_OK");
        led_ui_set_state(_LED_APP_MSG_POWER_ON_NAME, _LED_APP_MSG_POWER_ON_DISP_MODE);
        break;
    case BT_STATUS_FIRST_CONNECTED:
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_FIRST_CONNECTED");
    case BT_STATUS_SECOND_CONNECTED:

#if TCFG_BT_DUAL_CONN_ENABLE && _TCFG_BT_DUAL_CONN_ALL_DISCONNECT_ENABLE
        //每连接一次就加一次
        bt_total_connect_num++;
#endif

#if _LED_POWER_LOW_NO_INTERRUPT_ENABLE
        if (app_var.power_low_no_interrupt_flag){
            //蓝牙连接成功灯效不能打断低电灯效
            log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_FIRST_CONNECTED----power_low_no_interrupt_flag");
            break;
        }
#endif

        //蓝牙连接成功灯效
        app_msg_power_on_flag = 0; //连接成功后置0,超距断开就可以走APP_MSG_BT_IN_PAGE_MODE
        app_msg_bt_in_page_mode_flag = 0;//连接之后，超距灯效标志清除。
#if _LED_BT_STATUS_CONNECTED_LEFT_RIGHT
        //灯效分左右
        if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
            (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_LEFT_NAME, _LED_BT_STATUS_CONNECTED_LEFT_DISP_MODE);  
        }else {
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_RIGHT_NAME, _LED_BT_STATUS_CONNECTED_RIGHT_DISP_MODE);    
        }
#elif _LED_BT_STATUS_CONNECTED_MASTER_SLAVE
        //灯效分主从
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_MASTER_NAME, _LED_BT_STATUS_CONNECTED_MASTER_DISP_MODE);
        }else if(tws_api_get_role() == TWS_ROLE_SLAVE){
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_SLAVE_NAME, _LED_BT_STATUS_CONNECTED_SLAVE_DISP_MODE);
        }
#else
        //默认双耳同步灯效
        led_ui_set_state(_LED_BT_STATUS_CONNECTED_NAME, _LED_BT_STATUS_CONNECTED_DISP_MODE);
#endif
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_SECOND_CONNECTED");
        break;
    case BT_STATUS_A2DP_MEDIA_START:
        break;
    case BT_STATUS_PHONE_ACTIVE:
    case BT_STATUS_PHONE_HANGUP:
        //如果进入时开启了某种灯效，退出时一定要还原到蓝牙连接灯效
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_PHONE_HANGUP----BT_STATUS_PHONE_ACTIVE");
#if _LED_BT_STATUS_CONNECTED_LEFT_RIGHT
        //灯效分左右
        if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
            (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_LEFT_NAME, _LED_BT_STATUS_CONNECTED_LEFT_DISP_MODE);  
        }else {
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_RIGHT_NAME, _LED_BT_STATUS_CONNECTED_RIGHT_DISP_MODE);    
        }
#elif _LED_BT_STATUS_CONNECTED_MASTER_SLAVE
        //灯效分主从
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_MASTER_NAME, _LED_BT_STATUS_CONNECTED_MASTER_DISP_MODE);
        }else if(tws_api_get_role() == TWS_ROLE_SLAVE){
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_SLAVE_NAME, _LED_BT_STATUS_CONNECTED_SLAVE_DISP_MODE);
        }
#else
        //默认双耳同步灯效
        led_ui_set_state(_LED_BT_STATUS_CONNECTED_NAME, _LED_BT_STATUS_CONNECTED_DISP_MODE);
#endif
        break;
    case BT_STATUS_PHONE_INCOME:
        //如果进入时开启了某种灯效，退出时一定要还原到蓝牙连接灯效
        if (!esco_player_runing()) {
            led_ui_set_state(_LED_BT_STATUS_PHONE_INCOME_NAME, _LED_BT_STATUS_PHONE_INCOME_DISP_MODE);
        }
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_PHONE_INCOME-------------------------------------------------------");
        break;
    case BT_STATUS_SCO_CONNECTION_REQ:
        //如果进入时开启了某种灯效，退出时一定要还原到蓝牙连接灯效
        u8 call_status = bt_get_call_status_for_addr(bt->args);
        if (call_status == BT_CALL_INCOMING) {
            //来电，这个不修改，来电灯效同BT_STATUS_PHONE_INCOME，
            //这个没必要设置
            if (!esco_player_runing()) {
                //led_ui_set_state(LED_STA_BLUE_FAST_FLASH, DISP_TWS_SYNC);
                //led_ui_set_state(LED_STA_ALL_OFF, DISP_NON_INTR | DISP_TWS_SYNC);
                log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_SCO_CONNECTION_REQ----------BT_CALL_INCOMING---------");
            }
        } else if (call_status == BT_CALL_OUTGOING) {
            log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_SCO_CONNECTION_REQ----------BT_CALL_OUTGOING---------");
        }
        break;
    case BT_STATUS_A2DP_MEDIA_STOP:
        //如果进入时开启了某种灯效，退出时一定要还原到蓝牙连接灯效
        //停止音乐一般无灯效，先不设置
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_A2DP_MEDIA_STOP");
        break;
    case BT_STATUS_FIRST_DISCONNECT:
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_FIRST_DISCONNECT");
    case BT_STATUS_SECOND_DISCONNECT:
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_SECOND_DISCONNECT");

#if TCFG_BT_DUAL_CONN_ENABLE && _TCFG_BT_DUAL_CONN_ALL_DISCONNECT_ENABLE
        //每断开一次就减一次
        bt_total_connect_num--;

#endif
#if _LED_POWER_LOW_NO_INTERRUPT_ENABLE
        if (app_var.power_low_no_interrupt_flag){
            //断开灯效不能打断低电灯效
            log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_SECOND_DISCONNECT----power_low_no_interrupt_flag");
            break;
        }
#endif

        /*
         * 关机不执行断开灯效
         */
        if (app_var.goto_poweroff_flag) {
            //在关机流程中，这种变量会在灯效前被置1
            log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_FIRST_DISCONNECT------goto_poweroff_flag--------break-------");
            break;
        }

        //恢复出厂设置过程中也有蓝牙断开操作。断开之前标志位会被置1
        if(app_var.factory_reset_flag){
            log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_FIRST_DISCONNECT------factory_reset_flag--------break-------");
            break;
        }
        
#if TCFG_BT_DUAL_CONN_ENABLE && _TCFG_BT_DUAL_CONN_ALL_DISCONNECT_ENABLE        
        //一拖二的情况下只要有一台保持连接。就不执行蓝牙断开灯效
        //都不行：tws_state & TWS_STA_PHONE_CONNECTED----(bt_get_total_connect_dev()----bt_get_connect_status()
        if(bt_total_connect_num > 0){
            //正常如果断开所有连接设备的话应该是变成0了。不是一拖二也可以使用
            log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_FIRST_CONNECTED----bt_total_connect_num");
            break;
        }
#endif

        //蓝牙断开灯效
#if _LED_BT_STATUS_DISCONNECT_LEFT_RIGHT
        #if _LED_BT_STATUS_DISCONNECT_LEFT_RIGHT_ALONE_ENABLE
            //分左右时，单耳是否只闪烁一种灯效。
            if((tws_state & TWS_STA_SIBLING_DISCONNECTED)){
                led_ui_set_state(_LED_BT_STATUS_DISCONNECT_LEFT_RIGHT_ALONE_NAME, _LED_BT_STATUS_DISCONNECT_LEFT_RIGHT_ALONE_DISP_MODE);
                break;
            }
        #endif
        //蓝牙断开，灯效分左右
        if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
            (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
            led_ui_set_state(_LED_BT_STATUS_DISCONNECT_LEFT_NAME, _LED_BT_STATUS_DISCONNECT_LEFT_DISP_MODE);   
        }else {
            led_ui_set_state(_LED_BT_STATUS_DISCONNECT_RIGHT_NAME, _LED_BT_STATUS_DISCONNECT_RIGHT_DISP_MODE);    
        }
#elif _LED_BT_STATUS_DISCONNECT_MASTER_SLAVE
        //蓝牙断开，TWS状态下灯效分主从
        //非TWS走主机灯效
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            led_ui_set_state(_LED_BT_STATUS_DISCONNECT_MASTER_NAME, _LED_BT_STATUS_DISCONNECT_MASTER_DISP_MODE);
        }else if(tws_api_get_role() == TWS_ROLE_SLAVE){
            led_ui_set_state(_LED_BT_STATUS_DISCONNECT_SLAVE_NAME, _LED_BT_STATUS_DISCONNECT_SLAVE_DISP_MODE);
        }
#else
        //单耳时灯效
        if(tws_state & TWS_STA_SIBLING_DISCONNECTED){
            //单耳状态闪烁谁的灯效
            led_ui_set_state(_LED_BT_STATUS_DISCONNECT_TWS_STA_SIBLING_DISCONNECTED_NAME, _LED_BT_STATUS_DISCONNECT_TWS_STA_SIBLING_DISCONNECTED_DISP_MODE);   
        }else {
            //双耳同步灯效
            led_ui_set_state(_LED_BT_STATUS_DISCONNECT_NAME, _LED_BT_STATUS_DISCONNECT_DISP_MODE);
        }
#endif
        break;
    case BT_STATUS_SNIFF_STATE_UPDATE:
        //这个在蓝牙连接后周期性进入
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_SNIFF_STATE_UPDATE");
        break;
    }
    return 0;
}

#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
/**
 * @brief CIG连接状态事件处理函数
 *
 * @param msg:状态事件附带的返回参数
 *
 * @return
 */
static int led_ui_app_connected_conn_status_event_handler(int *msg)
{
    int *event = msg;
    /* g_printf("led_ui_app_connected_conn_status_event_handler=%d", event[0]); */

    switch (event[0]) {
    case CIG_EVENT_PERIP_CONNECT:
        /* log_info("CIG_EVENT_PERIP_CONNECT\n"); */
        /* led_ui_set_state(LED_STA_ALL_OFF, 0); */
        break;
    case CIG_EVENT_PERIP_DISCONNECT:
        break;
    case CIG_EVENT_ACL_CONNECT:
        break;
    case CIG_EVENT_ACL_DISCONNECT:
        break;
    case CIG_EVENT_JL_DONGLE_CONNECT:
    case CIG_EVENT_PHONE_CONNECT:
        log_info("CIG_EVENT_PHONE_CONNECT\n");
        led_ui_set_state(LED_STA_BLUE_ON_1S, DISP_NON_INTR);
        break;
    case CIG_EVENT_JL_DONGLE_DISCONNECT:
    case CIG_EVENT_PHONE_DISCONNECT:
        break;
    default:
        break;
    }

    return 0;
}
APP_MSG_PROB_HANDLER(app_le_connected_led_ui_msg_entry) = {
    .owner = 0xff,
    .from = MSG_FROM_CIG,
    .handler = led_ui_app_connected_conn_status_event_handler,
};
#endif

void ui_pwm_led_msg_handler(int *msg)
{
    struct led_state_obj *obj;

    switch (msg[0]) {
    case LED_MSG_STATE_END:
        log_info("MSG_FROM_PWM_LED----ui_pwm_led_msg_handler----LED_MSG_STATE_END");
        // 执行灯效结束消息, 可通过分配唯一的name来区分是哪个灯效
        //这里可以通过日志得知关机前的最后一个灯效名称
        obj = led_ui_get_state(msg[1]);
        if (obj) {
            log_info("LED_STATE_END: name = %d\n", obj->name);
            if (obj->name == LED_STA_POWERON) {

            }
        }
#if TCFG_USER_TWS_ENABLE
        led_tws_state_sync_stop();
#endif
        led_ui_state_machine_run();
        break;
    }

}

#if TCFG_USER_TWS_ENABLE
static int ui_tws_msg_handler(int *msg)
{
    struct tws_event *evt = (struct tws_event *)msg;
    u8 role = evt->args[0];
    u8 state = evt->args[1];
    u8 *bt_addr = evt->args + 3;

    char channel = tws_api_get_local_channel();
    int tws_state = tws_api_get_tws_state();

    switch (evt->event) {
    case TWS_EVENT_CONNECTED:
        //TWS连接灯效
        // if (role == TWS_ROLE_SLAVE) {
        //     led_ui_set_state(LED_STA_ALL_OFF, 0);
        // } else {
        //     struct led_state_obj *obj;
        //     obj = led_ui_get_first_state();
        //     if (obj && obj->disp_mode & DISP_TWS_SYNC) {
        //         led_ui_state_machine_run();
        //     }
        // }
#if _LED_POWER_LOW_NO_INTERRUPT_ENABLE
        if (app_var.power_low_no_interrupt_flag){
            //TWS灯效不能打断低电灯效
            log_info("MSG_FROM_TWS----ui_tws_msg_handler----TWS_EVENT_CONNECTED----power_low_no_interrupt_flag");
            break;
        }
#endif
        if(tws_state & TWS_STA_PHONE_DISCONNECTED){
            log_info("MSG_FROM_TWS----ui_tws_msg_handler----TWS_EVENT_CONNECTED------------------------------");
            //蓝牙没有连接的情况下，TWS连接与断开灯效才可以显示
#if _LED_TWS_EVENT_CONNECTED_LEFT_RIGHT
            if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                led_ui_set_state(_LED_TWS_EVENT_CONNECTED_LEFT_NAME, _LED_TWS_EVENT_CONNECTED_LEFT_DISP_MODE);    
            }else {
                led_ui_set_state(_LED_TWS_EVENT_CONNECTED_RIGHT_NAME, _LED_TWS_EVENT_CONNECTED_RIGHT_DISP_MODE);
            }
#elif _LED_TWS_EVENT_CONNECTED_MASTER_SLAVE
            //灯效分主从
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                led_ui_set_state(_LED_TWS_EVENT_CONNECTED_MASTER_NAME, _LED_TWS_EVENT_CONNECTED_MASTER_DISP_MODE);
            }else if(tws_api_get_role() == TWS_ROLE_SLAVE) {
                led_ui_set_state(_LED_TWS_EVENT_CONNECTED_SLAVE_NAME, _LED_TWS_EVENT_CONNECTED_SLAVE_DISP_MODE);
            }
#else
            //默认双耳同步灯效
            led_ui_set_state(_LED_TWS_EVENT_CONNECTED_NAME, _LED_TWS_EVENT_CONNECTED_DISP_MODE);
#endif
        }else if(tws_state & TWS_STA_PHONE_CONNECTED) {
            log_info("MSG_FROM_TWS----ui_tws_msg_handler----TWS_EVENT_CONNECTED---------bt_stack_msg_connected------TWS_STA_PHONE_CONNECTED---------");    
            //单耳已经连接手机，对耳再开机或者出仓，TWS连接，连接手机的最后case时TWS_EVENT_CONNECTED，中途不会进入蓝牙连接case
            //导致闪烁TWS_EVENT_CONNECTED灯效。
#if _LED_BT_STATUS_CONNECTED_LEFT_RIGHT
            //灯效分左右
            if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                led_ui_set_state(_LED_BT_STATUS_CONNECTED_LEFT_NAME, _LED_BT_STATUS_CONNECTED_LEFT_DISP_MODE);  
            }else {
                led_ui_set_state(_LED_BT_STATUS_CONNECTED_RIGHT_NAME, _LED_BT_STATUS_CONNECTED_RIGHT_DISP_MODE);    
            }
#elif _LED_BT_STATUS_CONNECTED_MASTER_SLAVE
            //灯效分主从
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                led_ui_set_state(_LED_BT_STATUS_CONNECTED_MASTER_NAME, _LED_BT_STATUS_CONNECTED_MASTER_DISP_MODE);
            }else if(tws_api_get_role() == TWS_ROLE_SLAVE){
                led_ui_set_state(_LED_BT_STATUS_CONNECTED_SLAVE_NAME, _LED_BT_STATUS_CONNECTED_SLAVE_DISP_MODE);
            }
#else
            //默认双耳同步灯效
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_NAME, _LED_BT_STATUS_CONNECTED_DISP_MODE);
#endif
        }
        break;
    case TWS_EVENT_CONNECTION_DETACH:
        extern u8 dut_on_flag;
#if _LED_POWER_LOW_NO_INTERRUPT_ENABLE
        if (app_var.power_low_no_interrupt_flag){
            //TWS灯效不能打断低电灯效
            log_info("MSG_FROM_TWS----ui_tws_msg_handler----TWS_EVENT_CONNECTION_DETACH----power_low_no_interrupt_flag");
            break;
        }
#endif
        /*
         * 关机不执行断开灯效
         */
        if (app_var.goto_poweroff_flag) {
            //在关机流程中，这种变量会在灯效前被置1
            log_info("MSG_FROM_TWS----ui_tws_msg_handler----TWS_EVENT_CONNECTION_DETACH------goto_poweroff_flag--------break-------");
            break;
        } else if(dut_on_flag == 1){
            //DUT模式下，TWS断开不执行断开灯效
            log_info("MSG_FROM_TWS----ui_tws_msg_handler----TWS_EVENT_CONNECTION_DETACH------dut_on_flag--------break-------");
            break;
        }
        if(tws_state & TWS_STA_PHONE_DISCONNECTED){
            //没有连接蓝牙的情况下才有效。
            //TWS断开灯效
            //断开后都是主耳，设置一种灯效就可以，一般跟未连接手机灯效一致。
            led_ui_set_state(_LED_TWS_EVENT_CONNECTION_DETACH_NAME, _LED_TWS_EVENT_CONNECTION_DETACH_DISP_MODE);
            led_tws_state_sync_stop();//注意此位置引起的灯效变化
            log_info("MSG_FROM_TWS----ui_tws_msg_handler----TWS_EVENT_CONNECTION_DETACH------------------------------");
        }   
        break;
    }
    return 0;
}
#endif



static int ui_led_msg_handler(int *msg)
{
    switch (msg[0]) {
#if TCFG_USER_TWS_ENABLE
    case MSG_FROM_TWS:
        //TWS对耳的灯效消息
        ui_tws_msg_handler(msg + 1);
        break;
#endif
    case MSG_FROM_BT_STACK:
        //跟蓝牙状态有关的灯效消息
        ui_bt_stack_msg_handler(msg + 1);
        break;
    case MSG_FROM_APP:
        //跟应用状态有关的灯效消息
        ui_app_msg_handler(msg + 1);
        break;
    case MSG_FROM_BATTERY:
        //电池相关的灯效消息
        ui_battery_msg_handler(msg + 1);
        break;
    case MSG_FROM_PWM_LED:
        ui_pwm_led_msg_handler(msg + 1);
        break;
    default:
        break;
    }
    return 0;
}

APP_MSG_HANDLER(led_msg_entry) = {
    .owner      = 0xff,
    .from       = 0xff,
    .handler    = ui_led_msg_handler,
};


/**
 * @brief 低延迟灯效,写到更新游戏模式的底层实现里，无论哪一个触发源都可以更新灯效
 * 
 */
void update_game_led(void){
#if _LED_POWER_LOW_NO_INTERRUPT_ENABLE
    if (app_var.power_low_no_interrupt_flag){
        //低延迟灯效不能打断低电灯效
        return;
    }
#endif
    led_ui_set_state(_LED_APP_MSG_LOW_LANTECY_NAME, _LED_APP_MSG_LOW_LANTECY_DISP_MODE);
    printf("%s, true\n", __FUNCTION__);
}
void update_normal_led(void){
#if _LED_POWER_LOW_NO_INTERRUPT_ENABLE
    if (app_var.power_low_no_interrupt_flag){
        //低延迟灯效不能打断低电灯效
        return;
    }
#endif
    int tws_state = tws_api_get_tws_state();
    char channel = tws_api_get_local_channel();
    //游戏模式退出时对状态进行判断
    //优先蓝牙连接灯效
    if(tws_state & TWS_STA_PHONE_CONNECTED){
    //已经是低延迟模式了，退出。还原到蓝牙连接灯效
#if _LED_BT_STATUS_CONNECTED_LEFT_RIGHT
        //灯效分左右
        if (channel == 'L') {
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_LEFT_NAME, _LED_BT_STATUS_CONNECTED_LEFT_DISP_MODE);  
        }else {
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_RIGHT_NAME, _LED_BT_STATUS_CONNECTED_RIGHT_DISP_MODE);    
        }
#elif _LED_BT_STATUS_CONNECTED_MASTER_SLAVE
        //灯效分主从
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_MASTER_NAME, _LED_BT_STATUS_CONNECTED_MASTER_DISP_MODE);
        }else if(tws_api_get_role() == TWS_ROLE_SLAVE){
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_SLAVE_NAME, _LED_BT_STATUS_CONNECTED_SLAVE_DISP_MODE);
        }
#else
        //默认双耳同步灯效
        led_ui_set_state(_LED_BT_STATUS_CONNECTED_NAME, _LED_BT_STATUS_CONNECTED_DISP_MODE);
#endif
    } else if(tws_state & TWS_STA_PHONE_DISCONNECTED){
        //其次是手机未连接状态
#if _LED_BT_STATUS_DISCONNECT_LEFT_RIGHT
        #if _LED_BT_STATUS_DISCONNECT_LEFT_RIGHT_ALONE_ENABLE
            //分左右时，单耳是否只闪烁一种灯效。
            if(tws_state & TWS_STA_SIBLING_DISCONNECTED){
                led_ui_set_state(_LED_BT_STATUS_DISCONNECT_LEFT_RIGHT_ALONE_NAME, _LED_BT_STATUS_DISCONNECT_LEFT_RIGHT_ALONE_DISP_MODE);
                return ;
            }
        #endif
        //蓝牙断开，灯效分左右
        if (channel == 'L') {
            led_ui_set_state(_LED_BT_STATUS_DISCONNECT_LEFT_NAME, _LED_BT_STATUS_DISCONNECT_LEFT_DISP_MODE);   
        }else {
            led_ui_set_state(_LED_BT_STATUS_DISCONNECT_RIGHT_NAME, _LED_BT_STATUS_DISCONNECT_RIGHT_DISP_MODE);    
        }
#elif _LED_BT_STATUS_DISCONNECT_MASTER_SLAVE
        //蓝牙断开，TWS状态下灯效分主从
        //非TWS走主机灯效
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            led_ui_set_state(_LED_BT_STATUS_DISCONNECT_MASTER_NAME, _LED_BT_STATUS_DISCONNECT_MASTER_DISP_MODE);
        }else if(tws_api_get_role() == TWS_ROLE_SLAVE){
            led_ui_set_state(_LED_BT_STATUS_DISCONNECT_SLAVE_NAME, _LED_BT_STATUS_DISCONNECT_SLAVE_DISP_MODE);
        }
#else
        //单耳时灯效
        if(tws_state & TWS_STA_SIBLING_DISCONNECTED){
            //单耳状态闪烁谁的灯效
            led_ui_set_state(_LED_BT_STATUS_DISCONNECT_TWS_STA_SIBLING_DISCONNECTED_NAME, _LED_BT_STATUS_DISCONNECT_TWS_STA_SIBLING_DISCONNECTED_DISP_MODE);   
        }else {
            //双耳同步灯效
            led_ui_set_state(_LED_BT_STATUS_DISCONNECT_NAME, _LED_BT_STATUS_DISCONNECT_DISP_MODE);
        }
#endif
    }
    printf("%s, false\n", __FUNCTION__);
}

#endif

