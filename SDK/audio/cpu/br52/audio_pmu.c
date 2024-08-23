
#include "typedef.h"
#include "asm/lpower_mem.h"
#include "audio_pmu.h"


void audio_dev_enter_low_power(enum audio_hw_device dev)
{
    switch (dev) {
    case AUDIO_HW_SBC_DEV:
        y_printf("===  dev_mem_sbc_enter_low_power ===\n");
        dev_mem_enter_low_power(DEV_SBC, DEV_MODE_PSD);
        break;
    default:
        printf("audio_dev_enter_low_power error!!! dev : %d\n ", dev);
        break;
    }

}

void audio_dev_exit_low_power(enum audio_hw_device dev)
{
    switch (dev) {
    case AUDIO_HW_SBC_DEV:
        y_printf("===  dev_mem_sbc_exit_low_power ===\n");
        dev_mem_exit_low_power(DEV_SBC, DEV_MODE_PSD);
        break;
    default:
        printf("audio_dev_exit_low_power error!!! dev : %d\n ", dev);
        break;
    }

}
