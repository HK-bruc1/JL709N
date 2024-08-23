
static void lp_touch_key_short_click_time_out_handle(void *priv)
{
    u8 ch = *((u8 *)priv);
    struct key_event e;
    switch (__this->click_cnt[ch]) {
    case 0:
        return;
        break;
    case 1:
        e.event = KEY_ACTION_CLICK;
        break;
    case 2:
        e.event = KEY_ACTION_DOUBLE_CLICK;
        break;
    case 3:
        e.event = KEY_ACTION_TRIPLE_CLICK;
        break;
    case 4:
        e.event = KEY_ACTION_FOURTH_CLICK;
        break;
    case 5:
        e.event = KEY_ACTION_FIRTH_CLICK;
        break;
    default:
        e.event = KEY_ACTION_NO_KEY;
        break;
    }
    e.value = __this->key[ch].key_value;

    log_debug("notify key%d short event, cnt: %d", ch, __this->click_cnt[ch]);
    lp_touch_key_notify_key_event(&e, ch);

    __this->short_timer[ch] = 0;
    __this->last_key[ch] = TOUCH_KEY_NULL;
    __this->click_cnt[ch] = 0;
}

static void lp_touch_key_short_click_handle(u8 ch)
{
    __this->last_key[ch] = TOUCH_KEY_SHORT_CLICK;
    if (__this->short_timer[ch] == 0) {
        __this->click_cnt[ch] = 1;
        __this->short_timer[ch] = usr_timeout_add((void *)&lp_touch_key_idx_table[ch], lp_touch_key_short_click_time_out_handle, __this->short_click_check_time, 1);
    } else {
        __this->click_cnt[ch]++;
        usr_timer_modify(__this->short_timer[ch], __this->short_click_check_time);
    }
}

static void lp_touch_key_raise_click_handle(u8 ch)
{
    struct key_event e = {0};
    if (__this->last_key[ch] >= TOUCH_KEY_LONG_CLICK) {
        e.event = KEY_ACTION_UP;
        e.value = __this->key[ch].key_value;
        lp_touch_key_notify_key_event(&e, ch);

        __this->last_key[ch] = TOUCH_KEY_NULL;
        log_debug("notify key HOLD UP event");
    } else {
        lp_touch_key_short_click_handle(ch);
    }
}

static void lp_touch_key_long_click_handle(u8 ch)
{
    __this->last_key[ch] = TOUCH_KEY_LONG_CLICK;

    struct key_event e;
    e.event = KEY_ACTION_LONG;
    e.value = __this->key[ch].key_value;

    lp_touch_key_notify_key_event(&e, ch);
}

static void lp_touch_key_hold_click_handle(u8 ch)
{
    __this->last_key[ch] = TOUCH_KEY_HOLD_CLICK;

    struct key_event e;
    e.event = KEY_ACTION_HOLD;
    e.value = __this->key[ch].key_value;

    lp_touch_key_notify_key_event(&e, ch);
}

static void lp_touch_key_slide_up_handle(u8 ch)
{
    struct key_event e;
    e.event = KEY_SLIDER_UP;
    e.value = __this->slide_mode_key_value;
    lp_touch_key_notify_key_event(&e, ch);
}

static void lp_touch_key_slide_down_handle(u8 ch)
{
    struct key_event e;
    e.event = KEY_SLIDER_DOWN;
    e.value = __this->slide_mode_key_value;
    lp_touch_key_notify_key_event(&e, ch);
}


