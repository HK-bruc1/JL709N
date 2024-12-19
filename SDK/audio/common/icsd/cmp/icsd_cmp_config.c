#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".icsd_cmp.data.bss")
#pragma data_seg(".icsd_cmp.data")
#pragma const_seg(".icsd_cmp.text.const")
#pragma code_seg(".icsd_cmp.text")
#endif

#include "app_config.h"
#include "audio_config_def.h"
#if (TCFG_AUDIO_ANC_EAR_ADAPTIVE_EN && \
	 TCFG_AUDIO_ANC_ENABLE && \
	 TCFG_AUDIO_ANC_EAR_ADAPTIVE_VERSION == ANC_EXT_V2)

#include "audio_anc.h"

#if ANC_EAR_ADAPTIVE_CMP_EN

#include "icsd_cmp.h"
#include "icsd_cmp_app.h"
#include "icsd_cmp_config.h"

const int cmp_objFunc_type = 1;
const float cmp_GAIN_LIMIT_ALL = 30;


int (*cmp_printf)(const char *format, ...) = _cmp_printf;

/***************************************************************/
// cmp
/***************************************************************/

const u8 icsd_cmp_type[] = { 3, 2, 4, 2, 2, 2, 2, 2, 2, 2 };
const float icsd_cmp_Vrange_M[] = {   // 62
    -19, -10, 1900, 2500, 0.8, 1.0,
    -15, 3, 120,   150,  0.8, 1.0,
    -50, -50, 20,  20,  3, 3,
    -6, 3, 200, 500,  0.5, 0.9,
    -6, 3, 550,  1000,  0.5, 1.0,
    1.25, 2.5,
    0, 0, 0, 0, 0, 0,
};
const float icsd_cmp_Biquad_init_M[] = { // 217
    -34.000,  4988,  1.0,
    //
    -16.285, 2494, 1.0,
    -8, 120, 1.0,
    -50,  20, 3.0,
    -1.95,  210, 0.8,
    -3.0,  563,  0.6,
    1.77,
};

const float icsd_cmp_Weight_M[] = { //354
    0.000, 0.000, 0.000, 8.000, 8.00,  8.00,  8.00,  5.00,  5.000, 5.000, 5.000, 5.000, 5.000, 5.000, 5.000,
    5.000, 5.000, 5.000, 5.000, 5.00,  5.00,  5.00,  5.00,  5.000, 5.000, 5.000, 5.000, 5.000, 5.000, 5.000,
    5.000, 5.000, 5.000, 5.000, 5.00,  5.00,  5.00,  5.00,  5.000, 5.000, 5.000, 5.000, 5.000, 5.000, 5.000,
    2.000, 2.000, 2.000, 2.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
};
const float icsd_cmp_Gold_csv_M[] = {
    -20, -20, -20, -20, -20, -20,  -20, -20, -20, -20, -20,  -20, -20, -20, -20,
    -20, -20, -20, -20, -20, -20,  -20, -20, -20, -20, -20,  -20, -20, -20, -20,
    -20, -20, -20, -20, -20, -20,  -20, -20, -20, -20, -20,  -20, -20, -20, -20,
    -20, -20, -20, -20, -20, -20,  -20, -20, -20, -20, -20,  -20, -20, -20, -20,
    -20, -20, -20, -20, -20, -20,  -20, -20, -20, -20, -20,  -20, -20, -20, -20,
};

const float icsd_cmp_degree_set_0[] = {
    3, -3, 50, 2200, 50, 2700, 5,
};

const int   cmp_IIR_NUM_FLEX = 5;
const int   cmp_IIR_NUM_FIX  = ANC_ADAPTIVE_CMP_ORDER - cmp_IIR_NUM_FLEX;
const int   cmp_IIR_COEF = cmp_IIR_NUM_FLEX * 3 + 1;
const float cmp_FSTOP_IDX = 2000;
const float cmp_FSTOP_IDX2 = 2000;

#endif
#endif
