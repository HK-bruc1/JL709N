#ifndef __LP_IPC_H__
#define __LP_IPC_H__

//=================================消息格式========================================

//消息buf大小
#define MAX_POOL		    128

//消息类型
#define NO_MSG				0xff

//获取消息返回值
enum {
    MSG_NO_ERROR = 0,
    MSG_NO_MSG = 0,
    MSG_EVENT_EXIST = -1,
    MSG_NOT_EVENT = -2,
    MSG_EVENT_PARAM_ERROR = -3,
    MSG_BUF_NOT_ENOUGH = -4,
    MSG_CBUF_ERROR = -5,
};

//消息头格式
#define MSG_HEADER_BYTE_LEN     3
#define MSG_HEADER_BIT_LEN     (MSG_HEADER_BYTE_LEN*8)
#define MSG_HEADER_ALL_BIT     ((1L<<MSG_HEADER_BIT_LEN) - 1)

#define MSG_INDEX_BIT			7
#define MSG_ACK_BIT  			1

#define MSG_TYPE_BIT_LEN        12
#define MSG_PARAM_BIT_LEN       (MSG_HEADER_BYTE_LEN*8-MSG_TYPE_BIT_LEN-MSG_INDEX_BIT-MSG_ACK_BIT)

//===========================================================================//
//                              P2M MESSAGE TABLE                            //
//===========================================================================//
//==================power=============================
#define P2M_WKUP_SRC                                    P2M_MESSAGE_ACCESS(0)
#define P2M_WKUP_P_PND                                  P2M_MESSAGE_ACCESS(1)
#define P2M_WKUP_N_PND                                  P2M_MESSAGE_ACCESS(2)
#define P2M_AWKUP_P_PND                                 P2M_MESSAGE_ACCESS(3)
#define P2M_AWKUP_N_PND                                 P2M_MESSAGE_ACCESS(4)
#define P2M_WKUP_RTC                                    P2M_MESSAGE_ACCESS(5)

//==================system===========================
#define P2M_MESSAGE_BANK_ADR_L                          P2M_MESSAGE_ACCESS(15)
#define P2M_MESSAGE_BANK_ADR_H                          P2M_MESSAGE_ACCESS(16)
#define P2M_MESSAGE_BANK_INDEX                          P2M_MESSAGE_ACCESS(17)
#define P2M_MESSAGE_BANK_ACK                            P2M_MESSAGE_ACCESS(18)
#define P2M_P11_HEAP_BEGIN_ADDR_L    					P2M_MESSAGE_ACCESS(19)
#define P2M_P11_HEAP_BEGIN_ADDR_H    					P2M_MESSAGE_ACCESS(20)
#define P2M_P11_HEAP_SIZE_L    							P2M_MESSAGE_ACCESS(21)
#define P2M_P11_HEAP_SIZE_H    							P2M_MESSAGE_ACCESS(22)
#define P2M_REPLY_SYNC_CMD                            	P2M_MESSAGE_ACCESS(23)
#define P2M_CBUF_ADDR0									P2M_MESSAGE_ACCESS(24)
#define P2M_CBUF_ADDR1                                  P2M_MESSAGE_ACCESS(25)
#define P2M_CBUF_ADDR2                                  P2M_MESSAGE_ACCESS(26)
#define P2M_CBUF_ADDR3                                  P2M_MESSAGE_ACCESS(27)
#define P2M_CBUF1_ADDR0                                 P2M_MESSAGE_ACCESS(28)
#define P2M_CBUF1_ADDR1                                 P2M_MESSAGE_ACCESS(29)
#define P2M_CBUF1_ADDR2                                 P2M_MESSAGE_ACCESS(30)
#define P2M_CBUF1_ADDR3                                 P2M_MESSAGE_ACCESS(31)


//==================lpctmu===========================
#define P2M_CTMU_CMD_ACK                                P2M_MESSAGE_ACCESS(39)
#define P2M_CTMU_KEY_EVENT                              P2M_MESSAGE_ACCESS(40)
#define P2M_CTMU_KEY_CNT                                P2M_MESSAGE_ACCESS(41)
#define P2M_CTMU_KEY_STATE                              P2M_MESSAGE_ACCESS(42)
#define P2M_CTMU_EARTCH_EVENT                           P2M_MESSAGE_ACCESS(43)

#define P2M_MASSAGE_CTMU_CH0_L_RES                                         44
#define P2M_MASSAGE_CTMU_CH0_H_RES                                         45
#define P2M_CTMU_CH0_L_RES                              P2M_MESSAGE_ACCESS(44)
#define P2M_CTMU_CH0_H_RES                              P2M_MESSAGE_ACCESS(45)
#define P2M_CTMU_CH1_L_RES                              P2M_MESSAGE_ACCESS(46)
#define P2M_CTMU_CH1_H_RES                              P2M_MESSAGE_ACCESS(47)
#define P2M_CTMU_CH2_L_RES                              P2M_MESSAGE_ACCESS(48)
#define P2M_CTMU_CH2_H_RES                              P2M_MESSAGE_ACCESS(49)
#define P2M_CTMU_CH3_L_RES                              P2M_MESSAGE_ACCESS(50)
#define P2M_CTMU_CH3_H_RES                              P2M_MESSAGE_ACCESS(51)
#define P2M_CTMU_CH4_L_RES                              P2M_MESSAGE_ACCESS(52)
#define P2M_CTMU_CH4_H_RES                              P2M_MESSAGE_ACCESS(53)

#define P2M_CTMU_EARTCH_L_IIR_VALUE                     P2M_MESSAGE_ACCESS(54)
#define P2M_CTMU_EARTCH_H_IIR_VALUE                     P2M_MESSAGE_ACCESS(55)
#define P2M_CTMU_EARTCH_L_DIFF_VALUE                    P2M_MESSAGE_ACCESS(56)
#define P2M_CTMU_EARTCH_H_DIFF_VALUE                    P2M_MESSAGE_ACCESS(57)


//===========================================================================//
//                              M2P MESSAGE TABLE                            //
//===========================================================================//
//==================power=============================
#define M2P_LRC_PRD                                     M2P_MESSAGE_ACCESS(0)
#define M2P_WDVDD                                		M2P_MESSAGE_ACCESS(1)
#define M2P_LRC_TMR_10us                                M2P_MESSAGE_ACCESS(2)
#define M2P_LRC_TMR_50us                                M2P_MESSAGE_ACCESS(3)
#define M2P_LRC_TMR_200us                               M2P_MESSAGE_ACCESS(4)
#define M2P_LRC_TMR_600us                               M2P_MESSAGE_ACCESS(5)
#define M2P_VDDIO_KEEP                                  M2P_MESSAGE_ACCESS(6)
#define M2P_LRC_KEEP                                    M2P_MESSAGE_ACCESS(7)
#define M2P_RCH_FEQ_L                                   M2P_MESSAGE_ACCESS(8)
#define M2P_RCH_FEQ_H                                   M2P_MESSAGE_ACCESS(9)
#define M2P_MEM_CONTROL                                 M2P_MESSAGE_ACCESS(10)
#define M2P_BTOSC_KEEP                                  M2P_MESSAGE_ACCESS(11)
#define M2P_CTMU_KEEP									M2P_MESSAGE_ACCESS(12)
#define M2P_RTC_KEEP                                    M2P_MESSAGE_ACCESS(13)
#define M2P_SF_MODE										M2P_MESSAGE_ACCESS(14)
#define M2P_LRC_FEQ_L_10Hz                              M2P_MESSAGE_ACCESS(15)
#define M2P_LRC_FEQ_H_10Hz                              M2P_MESSAGE_ACCESS(16)
#define M2P_LIGHT_PDOWN_DVDD_VOL						M2P_MESSAGE_ACCESS(17)
//==================system===========================
#define M2P_SYNC_CMD                                  	M2P_MESSAGE_ACCESS(25)
#define M2P_WDT_SYNC                                   	M2P_MESSAGE_ACCESS(26)



//==================lpctmu===========================
/*触摸所有通道配置*/
#define M2P_CTMU_CMD  									M2P_MESSAGE_ACCESS(50)
#define M2P_CTMU_CH_ENABLE								M2P_MESSAGE_ACCESS(51)
#define M2P_CTMU_CH_WAKEUP_EN					        M2P_MESSAGE_ACCESS(52)
#define M2P_CTMU_CH_DEBUG								M2P_MESSAGE_ACCESS(53)
#define M2P_CTMU_CH_CFG						            M2P_MESSAGE_ACCESS(54)
#define M2P_CTMU_EARTCH_CH						        M2P_MESSAGE_ACCESS(55)


#define M2P_CTMU_SCAN_TIME          					M2P_MESSAGE_ACCESS(56)
#define M2P_CTMU_LOWPOER_SCAN_TIME     					M2P_MESSAGE_ACCESS(57)
#define M2P_CTMU_LONG_KEY_EVENT_TIMEL                   M2P_MESSAGE_ACCESS(58)
#define M2P_CTMU_LONG_KEY_EVENT_TIMEH                   M2P_MESSAGE_ACCESS(59)
#define M2P_CTMU_HOLD_KEY_EVENT_TIMEL                   M2P_MESSAGE_ACCESS(60)
#define M2P_CTMU_HOLD_KEY_EVENT_TIMEH                   M2P_MESSAGE_ACCESS(61)
#define M2P_CTMU_SOFTOFF_WAKEUP_TIMEL                   M2P_MESSAGE_ACCESS(62)
#define M2P_CTMU_SOFTOFF_WAKEUP_TIMEH                   M2P_MESSAGE_ACCESS(63)
#define M2P_CTMU_LONG_PRESS_RESET_TIMEL  		        M2P_MESSAGE_ACCESS(64)//长按复位
#define M2P_CTMU_LONG_PRESS_RESET_TIMEH  		        M2P_MESSAGE_ACCESS(65)//长按复位

#define M2P_CTMU_INEAR_VALUE_L                          M2P_MESSAGE_ACCESS(66)
#define M2P_CTMU_INEAR_VALUE_H							M2P_MESSAGE_ACCESS(67)
#define M2P_CTMU_OUTEAR_VALUE_L                         M2P_MESSAGE_ACCESS(68)
#define M2P_CTMU_OUTEAR_VALUE_H                         M2P_MESSAGE_ACCESS(69)
#define M2P_CTMU_EARTCH_TRIM_VALUE_L                    M2P_MESSAGE_ACCESS(70)
#define M2P_CTMU_EARTCH_TRIM_VALUE_H                    M2P_MESSAGE_ACCESS(71)

#define M2P_MASSAGE_CTMU_CH0_CFG0L                                         72
#define M2P_MASSAGE_CTMU_CH0_CFG0H                                         73
#define M2P_MASSAGE_CTMU_CH0_CFG1L                                         74
#define M2P_MASSAGE_CTMU_CH0_CFG1H                                         75
#define M2P_MASSAGE_CTMU_CH0_CFG2L                                         76
#define M2P_MASSAGE_CTMU_CH0_CFG2H                                         77

#define M2P_CTMU_CH0_CFG0L                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG0L + 0 * 8))
#define M2P_CTMU_CH0_CFG0H                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG0H + 0 * 8))
#define M2P_CTMU_CH0_CFG1L                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG1L + 0 * 8))
#define M2P_CTMU_CH0_CFG1H                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG1H + 0 * 8))
#define M2P_CTMU_CH0_CFG2L                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG2L + 0 * 8))
#define M2P_CTMU_CH0_CFG2H                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG2H + 0 * 8))

#define M2P_CTMU_CH1_CFG0L                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG0L + 1 * 8))
#define M2P_CTMU_CH1_CFG0H                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG0H + 1 * 8))
#define M2P_CTMU_CH1_CFG1L                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG1L + 1 * 8))
#define M2P_CTMU_CH1_CFG1H                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG1H + 1 * 8))
#define M2P_CTMU_CH1_CFG2L                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG2L + 1 * 8))
#define M2P_CTMU_CH1_CFG2H                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG2H + 1 * 8))

#define M2P_CTMU_CH2_CFG0L                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG0L + 2 * 8))
#define M2P_CTMU_CH2_CFG0H                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG0H + 2 * 8))
#define M2P_CTMU_CH2_CFG1L                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG1L + 2 * 8))
#define M2P_CTMU_CH2_CFG1H                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG1H + 2 * 8))
#define M2P_CTMU_CH2_CFG2L                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG2L + 2 * 8))
#define M2P_CTMU_CH2_CFG2H                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG2H + 2 * 8))

#define M2P_CTMU_CH3_CFG0L                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG0L + 3 * 8))
#define M2P_CTMU_CH3_CFG0H                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG0H + 3 * 8))
#define M2P_CTMU_CH3_CFG1L                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG1L + 3 * 8))
#define M2P_CTMU_CH3_CFG1H                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG1H + 3 * 8))
#define M2P_CTMU_CH3_CFG2L                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG2L + 3 * 8))
#define M2P_CTMU_CH3_CFG2H                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG2H + 3 * 8))

#define M2P_CTMU_CH4_CFG0L                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG0L + 4 * 8))
#define M2P_CTMU_CH4_CFG0H                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG0H + 4 * 8))
#define M2P_CTMU_CH4_CFG1L                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG1L + 4 * 8))
#define M2P_CTMU_CH4_CFG1H                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG1H + 4 * 8))
#define M2P_CTMU_CH4_CFG2L                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG2L + 4 * 8))
#define M2P_CTMU_CH4_CFG2H                              M2P_MESSAGE_ACCESS((M2P_MASSAGE_CTMU_CH0_CFG2H + 4 * 8))//0x56



/*
 *  Must Sync to P11 code
 */
enum {
    M2P_LP_INDEX    = 0,
    M2P_PF_INDEX,
    M2P_LLP_INDEX,
    M2P_P33_INDEX,
    M2P_SF_INDEX,
    M2P_CTMU_INDEX,
    M2P_CCMD_INDEX,       //common cmd
    M2P_VAD_INDEX,
    M2P_USER_INDEX,
    M2P_WDT_INDEX,
    M2P_SYNC_INDEX,
    M2P_APP_INDEX,

};

enum {
    P2M_LP_INDEX    = 0,
    P2M_PF_INDEX,
    P2M_LLP_INDEX,
    P2M_WK_INDEX,
    P2M_WDT_INDEX,
    P2M_LP_INDEX2,
    P2M_CTMU_INDEX,
    P2M_CTMU_POWUP,
    P2M_REPLY_CCMD_INDEX,  //reply common cmd
    P2M_VAD_INDEX,
    P2M_USER_INDEX,
    P2M_BANK_INDEX,
    P2M_REPLY_SYNC_INDEX,
    P2M_APP_INDEX,
};

enum {
    CLOSE_P33_INTERRUPT = 1,
    OPEN_P33_INTERRUPT,
    LOWPOWER_PREPARE,
    M2P_SPIN_LOCK,
    M2P_SPIN_UNLOCK,
    P2M_SPIN_LOCK,
    P2M_SPIN_UNLOCK,

};

#include "m2p_msg.h"
#include "p2m_msg.h"


void msys_to_p11_sync_cmd(u8 cmd);

void lp_ipc_init();

#endif
