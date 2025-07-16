#include "system/includes.h"
#include "cpu/includes.h"
#include "app_config.h"



void sdpg_config(int enable)
{
    /* printf("******************   sdpg power = %d\n", enable); */
    if (enable) {
        JL_PORTB->OUT |=  BIT(5);
        JL_PORTB->DIR &= ~BIT(5);
        if (enable == 1) {
            p33_tx_1byte(P3_PGSD_CON, 0);
            JL_PORTB->HD0 &= ~BIT(5);
            JL_PORTB->HD1 &= ~BIT(5);
        } else if (enable == 2) {
            p33_tx_1byte(P3_PGSD_CON, 0);
            JL_PORTB->HD0 |=  BIT(5);
            udelay(500);
            JL_PORTB->HD1 |=  BIT(5);
            udelay(500);
        } else if (enable == 3) {
            JL_PORTB->HD0 |=  BIT(5);
            udelay(500);
            JL_PORTB->HD1 |=  BIT(5);
            udelay(500);
            p33_tx_1byte(P3_PGSD_CON, 0b0001);
        } else if (enable == 4) {
            JL_PORTB->HD0 |=  BIT(5);
            os_time_dly(1);
            JL_PORTB->HD1 |=  BIT(5);
            os_time_dly(1);
            p33_tx_1byte(P3_PGSD_CON, 0b0001);
        } else {
            p33_tx_1byte(P3_PGSD_CON, 0);
            JL_PORTB->PU0 &= ~BIT(5);
            JL_PORTB->PD0 |=  BIT(5);
            JL_PORTB->DIR |=  BIT(5);
            JL_PORTB->HD0 &= ~BIT(5);
            JL_PORTB->HD1 &= ~BIT(5);
            os_time_dly(5);
            JL_PORTB->HD0 |=  BIT(5);
            JL_PORTB->OUT &= ~BIT(5);
            JL_PORTB->DIR &= ~BIT(5);
            os_time_dly(2);
            JL_PORTB->HD0 &= ~BIT(5);
        }
    } else {
        p33_tx_1byte(P3_PGSD_CON, 0);
        JL_PORTB->DIR |=  BIT(5);
        JL_PORTB->PU0 &= ~BIT(5);
        JL_PORTB->PD0 &= ~BIT(5);
        JL_PORTB->HD0 &= ~BIT(5);
        JL_PORTB->HD1 &= ~BIT(5);
        JL_PORTB->DIE &= ~BIT(5);
    }
}


