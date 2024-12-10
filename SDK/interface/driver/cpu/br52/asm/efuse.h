#ifndef  __EFUSE_H__
#define  __EFUSE_H__


u32 efuse_get_gpadc_vbg_trim();
u32 get_chip_version();

u8 get_en_act();
u8 get_vddio_lvd_en();
u8 efuse_get_lvd_act();
u8 efuse_get_vddio_lvd_lev();
u8 efuse_get_vio_act();
u8 efuse_get_vddio_lev();
u8 efuse_get_mclr_en_dis();
u8 efuse_get_dvdd_cap_dis();
u8 efuse_get_sfc_fast_boot_dis();
u8 efuse_get_pin_reset_en();
u8 efuse_get_anc_enable();
u8 efuse_get_vbg_act();
u8 efuse_get_lvd_bg_trim();
u8 efuse_get_mvbg_lev();
u8 efuse_get_en_wvbg_lev();

u8 efuse_get_cp_pass();
u8 efuse_get_ft_pass();
u8 efuse_get_wvdd_level_trim();
u16 efuse_get_vbat_trim();
u16 efuse_get_vbat_trim_4p40(void);
u16 efuse_get_charge_cur_trim(void);
u16 efuse_get_io_pu_100k(void);
u16 efuse_get_vtemp(void);
u16 efuse_get_pmu_act(void);

u8 efuse_get_audio_vbg_trim();
u8 efuse_get_btvbg_bg_20k();
u8 efuse_get_bttx_pwr_trim();
u8 efuse_get_dcvdd12();
u8 efuse_get_bt_pll_pfd_ofst_act();
u8 efuse_get_bt_pll_pfd_ofst_sel();
u8 efuse_get_bt_cp_act();
u8 efuse_get_bt_ft_act();

u32 efuse_get_chip_id();
u16 get_chip_id();

u8 efuse_get_btvbg_xosc_trim();
u8 efuse_get_btvbg_syspll_trim();
u8 efuse_get_btvbg_xoscldo_level();
u8 efuse_get_btvbg_sysldo11d_level();
u8 efuse_get_btvbg_sysldo11a_level();
u32 efuse_get_vbat_3700();

void efuse_init();


#endif  /*EFUSE_H*/
