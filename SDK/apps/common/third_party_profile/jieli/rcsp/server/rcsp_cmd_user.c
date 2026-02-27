#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_cmd_user.data.bss")
#pragma data_seg(".rcsp_cmd_user.data")
#pragma const_seg(".rcsp_cmd_user.text.const")
#pragma code_seg(".rcsp_cmd_user.text")
#endif
#include "rcsp_cmd_user.h"
#include "app_config.h"
#include "ble_rcsp_server.h"

#if TCFG_AUDIO_SPATIAL_EFFECT_ENABLE
#include "spatial_effects_process.h"
#endif

#include "classic/tws_api.h"
#include "app_tone.h"


#if RCSP_MODE

#define RCSP_DEBUG_EN
#ifdef RCSP_DEBUG_EN
#define rcsp_putchar(x)                	putchar(x)
#define rcsp_printf                    	printf
#define rcsp_put_buf(x,len)				put_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_put_buf(...)
#endif

//*----------------------------------------------------------------------------*/
/**@brief    rcsp自定义命令数据接收处理
   @param    priv:全局rcsp结构体， OpCode:当前命令， OpCode_SN:当前的SN值， data:数据， len:数据长度
   @return
   @note	 二次开发需要增加自定义命令，通过JL_OPCODE_CUSTOMER_USER进行扩展，
  			 不要定义这个命令以外的命令，避免后续SDK更新导致命令冲突
*/
/*----------------------------------------------------------------------------*/
void rcsp_user_cmd_recieve(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    //自定义数据接收
    rcsp_printf("%s:", __FUNCTION__);
    rcsp_put_buf(data, len);
#if 0
    ///以下是发送测试代码
    u8 test_send_buf[] = {0x04, 0x05, 0x06};
    rcsp_user_cmd_send(test_send_buf, sizeof(test_send_buf));
#endif

    //调用自定义数据解析接口
    extern void rscp_user_cmd_recv(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
    rscp_user_cmd_recv(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);

    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);

}


//*----------------------------------------------------------------------------*/
/**@brief    rcsp自定义命令数据发送接口
   @param    data:数据， len:数据长度
   @return
   @note	 二次开发需要增加自定义命令，通过JL_OPCODE_CUSTOMER_USER进行扩展，
  			 不要定义这个命令以外的命令，避免后续SDK更新导致命令冲突
*/
/*----------------------------------------------------------------------------*/
JL_ERR rcsp_user_cmd_send(u8 *data, u16 len)
{
    //自定义数据接收
    rcsp_printf("%s:", __FUNCTION__);
    rcsp_put_buf(data, len);
    return JL_CMD_send(JL_OPCODE_CUSTOMER_USER, data, len, 1, 0, NULL);
}

//--------------------------自定义回复命令-----------------------------------------
void user_send_crtl_app_data(u8 type, u8 state)
{
    u8 send_date[6] = {0};
    send_date[0] = 0xAA;
    send_date[1] = 0xF1;
    send_date[2] = type;
    send_date[3] = state;
    send_date[4] = type + 1;
    send_date[5] = 0x55;
    rcsp_user_cmd_send(send_date,sizeof(send_date));
}

#define TWS_FUNC_ID_DIND_SYNC    TWS_FUNC_ID('T', 'I', 'N', 'D')
void bt_tws_sync_moed(u8 *value) //向副耳同步音效模式状态
{
    u8 data[2] = {0};
    data[0] = value[0];
    data[1] = value[1];
    printf("bt_tws_sync_moed---------data0:[%d]-data1:[%d]\r\n",data[0],data[1]);
    tws_api_send_data_to_slave(data, 2, TWS_FUNC_ID_DIND_SYNC);
}


//副耳在app_core任务中的操作
static void user_bt_tws_mode_sync(int cmd, int value)
{
    printf("tws_slave : state ---cmd:[%d]---value:[%d]-\r\n",cmd,value);
    u8 data[2] = {0};
    data[0] = (u8)cmd;
    data[1] = (u8)value;
    switch (data[0]) {
        case DHF_SPATIAL_SOUND_BEGIN:
    #if TCFG_AUDIO_SPATIAL_EFFECT_ENABLE
            audio_spatial_effects_mode_switch(data[1]);
    #endif
            break;

    }

    printf("tws_slave : end -------------------------------------------------------------------------");
}

static void user_app_protocol_tws_sync_rx(void *data, u16 len, bool rx)
{
    int err = 0;
    u8 *arg = (u8 *)data;
    int msg[8];
    printf("dhf_app_protocol_tws_sync_rx---data0:[%d]----data1[%d]--\r\n",arg[0],arg[1]);
    msg[0] = (int)user_bt_tws_mode_sync;
    msg[1] = 2;
    msg[2] = (int)arg[0]; //cmd
    msg[3] = (int)arg[1]; //value
    err = os_taskq_post_type("app_core", Q_CALLBACK, 4, msg);

    if (err) {
        printf("-----dhf tws sync post fail\n");
    }
}

REGISTER_TWS_FUNC_STUB(app_mode_sync_stub) = {
    .func_id = TWS_FUNC_ID_DIND_SYNC,
    .func    = user_app_protocol_tws_sync_rx,
};

//---------------------------------------------------------------------------------

//--------------------------数据解析 (APP发过来的数据)------------------------------
extern int get_bt_tws_connect_status();
extern int tws_play_tone_file(const char *file_name, int delay_msec);
extern const struct tone_files *get_tone_files();

void rscp_user_cmd_recv(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr){

    printf("接收到APP自定义指令----%s(%d)",__func__,__LINE__);
    rcsp_put_buf(data, len);

    u8 tws_sync_data[2] = {0};

    if ((data[0] == 0xAA) || (data[5] == 0x55)) {
        if (data[1] == 0xF0) {          
            switch(data[2]) {
                case DHF_SPATIAL_SOUND_BEGIN:
                    if (data[3] == DHF_SPATIAL_SOUND_INQUIRE) { //上报耳机模式
#if TCFG_AUDIO_SPATIAL_EFFECT_ENABLE
                        printf("dhf----get_a2dp_spatial_audio_mode():[%d]\r\n",get_a2dp_spatial_audio_mode());
                        data[3] = get_a2dp_spatial_audio_mode();
#endif
                    } else { //控制耳机
#if TCFG_AUDIO_SPATIAL_EFFECT_ENABLE
                        printf("%s : %d", __func__, data[3]);
                        
                        //提示音部分
                        if (data[3] == DHF_SPATIAL_SOUND_CLOSE) {//关音效
                            if (get_bt_tws_connect_status()) {
                                tws_play_tone_file(get_tone_files()->spatial_off, 400);
                            } else {
                                play_tone_file(get_tone_files()->spatial_off);
                            }
                        } else if (data[3] == DHF_SPATIAL_SOUND_OPEN) {//开音效
                            if (get_bt_tws_connect_status()) {
                                tws_play_tone_file(get_tone_files()->spatial_on, 400);
                            } else {
                                play_tone_file(get_tone_files()->spatial_on);
                            }
                        }
                        
                        //操作
                        if (get_bt_tws_connect_status()) {
                            tws_sync_data[0] = data[2];
                            tws_sync_data[1] = data[3];
                            bt_tws_sync_moed(tws_sync_data);
                        } else {
                            audio_spatial_effects_mode_switch(data[3]);
                        }
#endif
                    }
                    break;
                default:

                    break;
            }
            user_send_crtl_app_data(data[2],data[3]);//如果是查询命令则需要回复
        }
    }
}

#endif
