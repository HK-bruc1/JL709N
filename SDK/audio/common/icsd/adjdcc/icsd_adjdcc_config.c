#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".icsd_adt.data.bss")
#pragma data_seg(".icsd_adt.data")
#pragma const_seg(".icsd_adt.text.const")
#pragma code_seg(".icsd_adt.text")
#endif

#include "app_config.h"

#if ( TCFG_AUDIO_ANC_ENABLE && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN  && \
	TCFG_AUDIO_ADAPTIVE_DCC_ENABLE )

#include "icsd_adjdcc.h"
#include "icsd_adt.h"

int (*dcc_printf)(const char *format, ...) = _dcc_printf;

void anc_core_ff_adjdcc_par_set(u8 dc_par);

float err_overload_list[] = {90, 90, 85, 80};

void adjdcc_config_init(__adjdcc_config *_adjdcc_config)
{
    __adjdcc_config *adjdcc_config = _adjdcc_config;

    adjdcc_config->release_cnt = 6;
    adjdcc_config->err_overload_list = err_overload_list;
    adjdcc_config->refmic_max_thr = 300;
    adjdcc_config->refmic_mp_thr = 10;
    adjdcc_config->ff_dc_par = 6;
}

const struct adjdcc_function ADJDCC_FUNC_t = {
    .adjdcc_config_init = adjdcc_config_init,
    .ff_adjdcc_par_set = anc_core_ff_adjdcc_par_set,
};
struct adjdcc_function *ADJDCC_FUNC = (struct adjdcc_function *)(&ADJDCC_FUNC_t);

void icsd_adjdcc_demo()
{
    static int *alloc_addr = NULL;
    struct icsd_adjdcc_libfmt libfmt;
    struct icsd_adjdcc_infmt  fmt;
    icsd_adjdcc_get_libfmt(&libfmt);
    dcc_printf("ADJDCC RAM SIZE:%d\n", libfmt.lib_alloc_size);
    if (alloc_addr == NULL) {
        alloc_addr = zalloc(libfmt.lib_alloc_size);
    }
    fmt.alloc_ptr = alloc_addr;
    icsd_adjdcc_set_infmt(&fmt);
}

#endif

