#ifndef __RCSP_CMD_USER_H__
#define __RCSP_CMD_USER_H__

#include "typedef.h"
#include "JL_rcsp_protocol.h"

//空间音效
#define DHF_SPATIAL_SOUND_BEGIN         0xA5
#define DHF_SPATIAL_SOUND_END           0xA6
#define DHF_SPATIAL_SOUND_OPEN          0x01    //开
#define DHF_SPATIAL_SOUND_CLOSE         0x00    //关
#define DHF_SPATIAL_SOUND_INQUIRE       0x05    //app下发查询指令

//用户自定义数据接收
void rcsp_user_cmd_recieve(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
//用户自定义数据发送
JL_ERR rcsp_user_cmd_send(u8 *data, u16 len);

#endif // __RCSP_CMD_USER_H__
