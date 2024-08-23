
/******************************双通道滑动触摸识别的基础原理***********************************

1.单击，但如果超过设定的长按时间，那就为长按
chx  __________________________________________
chx  ___________                     __________
                |_______>180________|


2.单击，但如果超过设定的长按时间，那就为长按
chx  ___________                     __________
                |_______>180________|
chx  _______ <30                     <30 ______
            |___________________________|


3.单击，但如果超过设定的长按时间，那就为长按
chx  ___________                     __________
                |_______>180________|
chx  _______ <30                       >30   __
            |_______________________________|


4.单击，但如果超过设定的长按时间，那就为长按
chx  ___________                     __________
                |_______>180________|
chx  ___   >30                       <30 ______
        |_______________________________|


5.单击，但如果超过设定的长按时间，那就为长按
chx  ___________                     __________
                |_______>180________|
chx  ___   >30                         >30   __
        |___________________________________|


6.单击，但如果超过设定的长按时间，那就为长按
chx  ___________                         ______
                |_______________________|
chx  _______ <30        >180         _<30______
            |_______________________|


7.根据前后操作，有可能是滑动，也有可能是连击中的后单击，也有可能是无效操作
chx  ___________                 __________
                |_______________|
chx  _______ <30    <180     _<30__________
            |_______________|


8.滑动，t < 设定的长按的时间
chx  ___________                 __________
                |_______________|
chx  ___   >30       t       _<30__________
        |___________________|


9.滑动，t < 设定的长按的时间
chx  ___________                     ______
                |___________________|
chx  ___   >30       t       __>30_________
        |___________________|


10.滑动，t < 设定的长按的时间
chx  ___________                     ______
                |___________________|
chx  _______ <30     t       __>30_________
            |_______________|

*****************************************************************************/

enum falling_order_type {
    FALLING_NULL,
    FALLING_FIRST,
    FALLING_SECOND,
};
enum raising_order_type {
    RAISING_NULL,
    RAISING_FIRST,
    RAISING_SECOND,
};

enum time_interval_type {
    LONG_TIME,
    SHORT_TIME,
};

enum slide_key_type {
    SHORT_CLICK = 1,
    DOUBLE_CLICK,
    TRIPLE_CLICK,
    FOURTH_CLICK,
    FIRTH_CLICK,
    LONG_CLICK = 8,
    LONG_HOLD_CLICK = 9,
    LONG_UP_CLICK = 10,
    SLIDE_UP = 0x12,
    SLIDE_DOWN = 0x21,
};

static u8 falling_order[2] = {FALLING_NULL, FALLING_NULL};
static u8 raising_order[2] = {RAISING_NULL, RAISING_NULL};
static u8 falling_time_interval = 0;
static u8 raising_time_interval = 0;
static u8 last_key_type = 0;

#define FALLING_TIMEOUT_TIME    30              //两个按下边沿的时间间隔的阈值
static int falling_timeout_add = 0;
static void lp_touch_key_falling_timeout_handle(void *priv)
{
    falling_timeout_add = 0;
}

#define RAISING_TIMEOUT_TIME    30              //两个抬起边沿的时间间隔的阈值
static int raising_timeout_add = 0;
static void lp_touch_key_raising_timeout_handle(void *priv)
{
    raising_timeout_add = 0;
}

#define CLICK_IDLE_TIMEOUT_TIME 500             //如果该时间内，没有单击，那么该时间之后的第一次单击，识别为首次单击
static int click_idle_timeout_add = 0;
static void lp_touch_key_click_idle_timeout_handle(void *priv)
{
    click_idle_timeout_add = 0;
}

#define FIRST_SHORT_CLICK_TIMEOUT_TIME  190     //只针对首次单击，要满足按够那么长时间。连击时，后面的单击没有该时间要求
static int first_short_click_timeout_add = 0;
static void lp_touch_key_first_short_click_timeout_handle(void *priv)
{
    first_short_click_timeout_add = 0;
}

#define SEND_KEYTONE_TIMEOUT_TIME   250         //长按时的按键音，在按下后的多长时间要响
static u8 send_keytone_flag = 0;
static int send_keytone_timeout_add = 0;
static void lp_touch_key_send_keytone_timeout_handle(void *priv)
{
    lp_touch_key_send_key_tone_msg();
    send_keytone_timeout_add = 0;
    send_keytone_flag = 1;
}

#define SLIDE_CLICK_TIMEOUT_TIME    500         //识别为滑动之后，该时间内，如果有case7.也要识别为滑动
#define FAST_SLIDE_CNT_MAX          4           //一来就连续那么多次的case7.，就会识别为一次滑动。
static u8 fast_slide_cnt = 0;
static u8 fast_slide_type = 0;
static int slide_click_timeout_add = 0;
void lp_touch_key_slide_click_timeout_handle(void *priv)
{
    slide_click_timeout_add = 0;
}

static void lp_touch_key_check_channel_falling_info(u8 ch)
{
    falling_order[ch] = FALLING_FIRST;
    if (falling_order[!ch] == FALLING_FIRST) {
        falling_order[ch] = FALLING_SECOND;
    }
    send_keytone_flag = 0;
    if (send_keytone_timeout_add == 0) {
        send_keytone_timeout_add = sys_hi_timeout_add(NULL, lp_touch_key_send_keytone_timeout_handle, SEND_KEYTONE_TIMEOUT_TIME);
    } else {
        sys_hi_timer_modify(send_keytone_timeout_add, SEND_KEYTONE_TIMEOUT_TIME);
    }
    if ((last_key_type != SHORT_CLICK) || (click_idle_timeout_add == 0)) {
        if (first_short_click_timeout_add == 0) {
            first_short_click_timeout_add = sys_hi_timeout_add(NULL, lp_touch_key_first_short_click_timeout_handle, FIRST_SHORT_CLICK_TIMEOUT_TIME);
        } else {
            sys_hi_timer_modify(first_short_click_timeout_add, FIRST_SHORT_CLICK_TIMEOUT_TIME);
        }
    }
    if (falling_order[ch] == FALLING_FIRST) {
        if (falling_timeout_add == 0) {
            falling_timeout_add = sys_hi_timeout_add(NULL, lp_touch_key_falling_timeout_handle, FALLING_TIMEOUT_TIME);
        } else {
            sys_hi_timer_modify(falling_timeout_add, FALLING_TIMEOUT_TIME);
        }
        falling_time_interval = SHORT_TIME;
    } else if (falling_order[ch] == FALLING_SECOND) {
        if (falling_timeout_add) {
            sys_hi_timeout_del(falling_timeout_add);
            falling_timeout_add = 0;
            falling_time_interval = SHORT_TIME;
        } else {
            falling_time_interval = LONG_TIME;
        }
    }
}

static u8 lp_touch_key_check_channel_raising_info_and_key_type(u8 ch)
{
    u8 key_type = 0;
    fast_slide_type = 0;
    if (falling_order[ch] == FALLING_NULL) {
        return key_type;
    }
    raising_order[ch] = RAISING_FIRST;
    if (raising_order[!ch] == RAISING_FIRST) {
        raising_order[ch] = RAISING_SECOND;
    }
    if (falling_order[ch] > falling_order[!ch]) {
        if (raising_order[ch] == RAISING_FIRST) {
            key_type = SHORT_CLICK;             //case 1~5.
        } else {
            if (falling_time_interval == SHORT_TIME) {
                if (raising_timeout_add) {
                    sys_hi_timeout_del(raising_timeout_add);
                    raising_timeout_add = 0;
                    key_type = SHORT_CLICK;     //case 6~7.
                    if (ch == 0) {
                        fast_slide_type = SLIDE_DOWN;
                    } else {
                        fast_slide_type = SLIDE_UP;
                    }
                } else if (ch == 0) {
                    key_type = SLIDE_DOWN;      //case 10.
                } else {
                    key_type = SLIDE_UP;        //case 10.
                }
            } else if (ch == 0) {
                key_type = SLIDE_DOWN;          //caee 8~9.
            } else {
                key_type = SLIDE_UP;            //case 8~9.
            }
        }
    } else {
        if (raising_order[ch] == RAISING_FIRST) {
            if (falling_time_interval == SHORT_TIME) {
                if (raising_timeout_add == 0) {
                    raising_timeout_add = sys_hi_timeout_add(NULL, lp_touch_key_raising_timeout_handle, RAISING_TIMEOUT_TIME);
                } else {
                    sys_hi_timer_modify(raising_timeout_add, RAISING_TIMEOUT_TIME);
                }
            } else if (ch == 0) {
                key_type = SLIDE_UP;            //case 8~9.
            } else {
                key_type = SLIDE_DOWN;          //case 8~9.
            }
        } else {
            if (falling_time_interval == SHORT_TIME) {
                if (raising_timeout_add) {
                    sys_hi_timeout_del(raising_timeout_add);
                    raising_timeout_add = 0;
                    key_type = SHORT_CLICK;     //case 6~7
                    if (ch == 0) {
                        fast_slide_type = SLIDE_UP;
                    } else {
                        fast_slide_type = SLIDE_DOWN;
                    }
                } else if (ch == 0) {
                    key_type = SLIDE_UP;        //case 10.
                } else {
                    key_type = SLIDE_DOWN;      //case 10.
                }
            } else if (ch == 0) {
                key_type = SLIDE_UP;            //case 8~9.
            } else {
                key_type = SLIDE_DOWN;          //case 8~9.
            }
        }
    }
    return key_type;
}

static u8 lp_touch_key_get_slide_event_ch(void)
{
    u8 event_ch = 1;
    for (u8 ch = 0; ch < LPCTMU_CHANNEL_SIZE; ch ++) {
        if (__this->key[ch].enable) {
            event_ch = ch;
            break;
        }
    }
    return event_ch;
}

static u8 lp_touch_key_check_slide_key_type(u8 event, u8 ch)
{
    u8 key_type = 0;
    u8 event_ch = lp_touch_key_get_slide_event_ch();
    if (event == (CTMU_P2M_CH0_FALLING_EVENT + ch * 8)) {
        log_debug("touch key%d FALLING !\n", ch);
        if (ch == event_ch) {
            lp_touch_key_check_channel_falling_info(0);
        } else {
            lp_touch_key_check_channel_falling_info(1);
        }
    } else if (event == (CTMU_P2M_CH0_RAISING_EVENT + ch * 8)) {
        log_debug("touch key%d RAISING !\n", ch);
        if (ch == event_ch) {
            if ((last_key_type == LONG_CLICK) || (last_key_type == LONG_HOLD_CLICK)) {
                key_type = LONG_UP_CLICK;       //一定是长按抬起
            } else {
                key_type = lp_touch_key_check_channel_raising_info_and_key_type(0);
            }
        } else {
            key_type = lp_touch_key_check_channel_raising_info_and_key_type(1);
        }
    } else if (event == (CTMU_P2M_CH0_LONG_KEY_EVENT + event_ch * 8)) {//长按只判断通道号小的那个按键
        log_debug("touch key%d LONG !\n", ch);
        key_type = LONG_CLICK;
    } else if (event == (CTMU_P2M_CH0_HOLD_KEY_EVENT + event_ch * 8)) {//长按只判断通道号小的那个按键
        log_debug("touch key%d HOLD !\n", ch);
        key_type = LONG_HOLD_CLICK;
    }

    if (key_type) {
        falling_order[0] = FALLING_NULL;
        falling_order[1] = FALLING_NULL;
        raising_order[0] = RAISING_NULL;
        raising_order[1] = RAISING_NULL;
        falling_time_interval = 0;
        last_key_type = key_type;
        if (send_keytone_timeout_add) {
            sys_hi_timeout_del(send_keytone_timeout_add);
            send_keytone_timeout_add = 0;
        }
        if (key_type == SHORT_CLICK) {                  //case 6~7 的进一步处理
            if (first_short_click_timeout_add) {
                sys_hi_timeout_del(first_short_click_timeout_add);
                first_short_click_timeout_add = 0;

                if (slide_click_timeout_add) {
                    sys_hi_timeout_del(slide_click_timeout_add);
                    slide_click_timeout_add = 0;
                    if (fast_slide_type) {
                        key_type = fast_slide_type;
                        last_key_type = key_type;
                    } else {
                        key_type = 0xff;
                        last_key_type = key_type;
                    }
                    fast_slide_cnt = 0;
                } else {
                    if (fast_slide_type) {
                        fast_slide_cnt ++;
                        if (fast_slide_cnt >= FAST_SLIDE_CNT_MAX) {
                            fast_slide_cnt = 0;
                            key_type = fast_slide_type;
                            last_key_type = key_type;
                        } else {
                            key_type = 0xff;
                            last_key_type = key_type;
                        }
                    } else {
                        fast_slide_cnt = 0;
                        key_type = 0xff;
                        last_key_type = key_type;
                    }
                }
            } else {
                if (send_keytone_flag == 0) {
                    lp_touch_key_send_key_tone_msg();
                }
                if (slide_click_timeout_add) {
                    sys_hi_timeout_del(slide_click_timeout_add);
                    slide_click_timeout_add = 0;
                }
                fast_slide_cnt = 0;
            }
            if (click_idle_timeout_add == 0) {
                click_idle_timeout_add = sys_hi_timeout_add(NULL, lp_touch_key_click_idle_timeout_handle, CLICK_IDLE_TIMEOUT_TIME);
            } else {
                sys_hi_timer_modify(click_idle_timeout_add, CLICK_IDLE_TIMEOUT_TIME);
            }
        } else {
            fast_slide_cnt = 0;
        }

        if ((key_type == SLIDE_UP) || (key_type == SLIDE_DOWN)) {
            if (slide_click_timeout_add == 0) {
                slide_click_timeout_add = sys_hi_timeout_add(NULL, lp_touch_key_slide_click_timeout_handle, SLIDE_CLICK_TIMEOUT_TIME);
            } else {
                sys_hi_timer_modify(slide_click_timeout_add, SLIDE_CLICK_TIMEOUT_TIME);
            }
        }
    }

    return key_type;
}

static void lp_touch_key_send_slide_key_type_event(u8 key_type)
{
    u8 event_ch = lp_touch_key_get_slide_event_ch();
    switch (key_type) {
    case SHORT_CLICK:           //单击
        lp_touch_key_short_click_handle(event_ch);
        break;
    case LONG_CLICK:            //长按
        lp_touch_key_long_click_handle(event_ch);
        break;
    case LONG_HOLD_CLICK:       //长按保持
        lp_touch_key_hold_click_handle(event_ch);
        break;
    case LONG_UP_CLICK:         //长按抬起
        lp_touch_key_raise_click_handle(event_ch);
        break;
    case SLIDE_UP:              //向上滑动
        lp_touch_key_slide_up_handle(event_ch);
        break;
    case SLIDE_DOWN:            //向下滑动
        lp_touch_key_slide_down_handle(event_ch);
        break;
    default:
        break;
    }
}

