


























c_SRC_FILES += audio/framework/plugs/source/a2dp_file.c audio/framework/plugs/source/a2dp_streamctrl.c audio/framework/plugs/source/esco_file.c audio/framework/plugs/source/adc_file.c audio/framework/plugs/source/multi_ch_adc_file.c audio/framework/nodes/esco_tx_node.c audio/framework/nodes/plc_node.c audio/framework/nodes/volume_node.c
c_SRC_FILES += audio/framework/nodes/cvp_dms_node.c
c_SRC_FILES += audio/test_tools/audio_dut_control.c audio/test_tools/audio_dut_control_old.c





c_SRC_FILES += audio/common/audio_node_config.c audio/common/audio_dvol.c audio/common/audio_general.c audio/common/audio_build_needed.c audio/common/online_debug/aud_data_export.c audio/common/online_debug/audio_online_debug.c audio/common/online_debug/audio_capture.c audio/common/audio_plc.c audio/common/audio_noise_gate.c audio/common/audio_ns.c audio/common/audio_utils.c audio/common/audio_export_demo.c audio/common/amplitude_statistic.c audio/common/frame_length_adaptive.c audio/common/bt_audio_energy_detection.c audio/common/audio_event_handler.c audio/common/debug/audio_debug.c audio/common/power/mic_power_manager.c audio/common/audio_volume_mixer.c audio/common/audio_effect_verify.c audio/common/pcm_data/sine_pcm.c
c_SRC_FILES += audio/common/demo/hw_math_v2_demo.c
c_SRC_FILES += audio/interface/player/tone_player.c audio/interface/player/ring_player.c audio/interface/player/a2dp_player.c audio/interface/player/esco_player.c audio/interface/player/key_tone_player.c audio/interface/player/dev_flow_player.c audio/interface/player/adda_loop_player.c audio/interface/player/linein_player.c audio/interface/player/ai_rx_player.c
c_SRC_FILES += audio/interface/recoder/esco_recoder.c audio/interface/recoder/ai_voice_recoder.c audio/interface/recoder/dev_flow_recoder.c
c_SRC_FILES += audio/effect/eq_config.c audio/effect/spk_eq.c audio/effect/audio_voice_changer_api.c audio/effect/esco_ul_voice_changer.c audio/effect/bass_treble.c audio/effect/audio_dc_offset_remove.c audio/effect/effects_adj.c audio/effect/effects_dev.c audio/effect/effects_default_param.c audio/effect/node_param_update.c audio/effect/scene_update.c
c_SRC_FILES += audio/common/icsd/adt/icsd_adt.c audio/common/icsd/adt/icsd_adt_app.c audio/common/icsd/adt/icsd_adt_config.c audio/common/icsd/adt/icsd_adt_alg.c audio/common/icsd/adt/icsd_adt_demo.c audio/common/icsd/anc/icsd_anc_app.c audio/common/icsd/anc/icsd_anc_board.c audio/common/icsd/anc/icsd_anc_data.c audio/common/icsd/anc/icsd_anc_interactive.c audio/common/icsd/common/icsd_common.c audio/common/icsd/dot/icsd_dot_app.c audio/common/icsd/dot/icsd_dot.c audio/common/icsd/common_v2/icsd_common_v2.c audio/common/icsd/common_v2/icsd_common_v2_app.c audio/common/icsd/anc_v2/icsd_anc_v2.c audio/common/icsd/anc_v2/icsd_anc_v2_app.c audio/common/icsd/anc_v2/icsd_anc_v2_interactive.c audio/common/icsd/config/icsd_anc_v2_config.c audio/common/icsd/rt_anc/rt_anc.c audio/common/icsd/rt_anc/rt_anc_app.c audio/common/icsd/rt_anc/rt_anc_config.c audio/common/icsd/tool/anc_ext_tool.c audio/common/icsd/tool/anc_ext_tool_file.c audio/common/icsd/aeq/icsd_aeq_app.c audio/common/icsd/aeq/icsd_aeq.c audio/common/icsd/aeq/icsd_aeq_config.c audio/common/icsd/afq/icsd_afq_app.c audio/common/icsd/afq/icsd_afq.c audio/common/icsd/afq/icsd_afq_config.c audio/common/icsd/cmp/icsd_cmp_app.c audio/common/icsd/config/icsd_cmp_config.c audio/common/icsd/demo/icsd_demo.c audio/common/icsd/ein/icsd_ein_config.c audio/common/icsd/vdt/icsd_vdt_config.c audio/common/icsd/vdt/icsd_vdt_app.c audio/common/icsd/wat/icsd_wat_config.c audio/common/icsd/wat/icsd_wat_app.c audio/common/icsd/wind/icsd_wind_config.c audio/common/icsd/wind/icsd_wind_app.c audio/common/icsd/wind/cvp_wind_app.c audio/common/icsd/avc/icsd_avc_config.c audio/common/icsd/avc/icsd_avc_app.c audio/common/icsd/env_noise_det/icsd_env_noise_det_app.c audio/common/icsd/adjdcc/icsd_adjdcc_config.c audio/common/icsd/howl/icsd_howl_config.c audio/common/icsd/howl/icsd_howl_app.c
c_SRC_FILES += audio/anc/audio_anc_fade_ctr.c audio/anc/audio_anc_common_plug.c audio/anc/audio_anc_debug_tool.c audio/anc/audio_anc_mult_scene.c audio/anc/audio_anc_common.c audio/anc/anc_memory.c audio/anc/audio_anc_develop.c audio/anc/audio_anc_hw_src.c audio/anc/audio_anc_manager.c
c_SRC_FILES += audio/effect/somatosensory/audio_somatosensory.c





c_SRC_FILES += audio/CVP/audio_aec.c audio/CVP/audio_cvp.c audio/CVP/audio_cvp_dms.c audio/CVP/audio_cvp_3mic.c audio/CVP/audio_cvp_v3.c audio/CVP/audio_cvp_online.c audio/CVP/audio_cvp_demo.c audio/CVP/audio_cvp_develop.c audio/CVP/audio_cvp_sync.c audio/CVP/audio_cvp_ais_3mic.c audio/CVP/audio_cvp_ref_task.c audio/CVP/audio_cvp_config.c
c_SRC_FILES += audio/interface/player/tws_tone_player.c
c_SRC_FILES += audio/framework/plugs/source/linein_file.c
c_SRC_FILES += audio/cpu/common.c
c_SRC_FILES += apps/common/config/bt_profile_config.c
c_SRC_FILES += apps/common/update/update.c apps/common/update/testbox_update.c apps/common/update/testbox_uart_update.c
c_SRC_FILES += apps/common/ui/pwm_led/led_ui_api.c apps/common/ui/pwm_led/led_ui_tws_sync.c
c_SRC_FILES += apps/common/third_party_profile/multi_protocol_common.c apps/common/third_party_profile/multi_protocol_event.c
c_SRC_FILES += apps/common/third_party_profile/jieli/online_db/spp_online_db.c apps/common/third_party_profile/jieli/online_db/online_db_deal.c
c_SRC_FILES += apps/common/device/key/key_driver.c
c_SRC_FILES += apps/common/device/usb/device/usb_pll_trim.c
c_SRC_FILES += apps/common/device/usb/device/msd_upgrade.c
c_SRC_FILES += cpu/components/iic_soft.c cpu/components/iic_api.c cpu/components/ir_encoder.c cpu/components/ir_decoder.c cpu/components/rdec_soft.c







c_SRC_FILES += cpu/components/led_api.c cpu/components/two_io_led.c






c_SRC_FILES += cpu/components/touch/lp_touch_key_tool.c






c_SRC_FILES += cpu/config/gpio_file_parse.c cpu/config/lib_power_config.c
c_SRC_FILES += audio/cpu/br52/audio_setup.c audio/cpu/br52/audio_dai/audio_pdm.c audio/cpu/br52/audio_config.c audio/cpu/br52/audio_pmu.c audio/cpu/br52/audio_configs_dump.c







c_SRC_FILES += audio/cpu/br52/audio_anc.c audio/cpu/br52/icsd_anc_user.c






c_SRC_FILES += audio/cpu/br52/audio_accelerator/hw_fft.c



c_SRC_FILES += audio/cpu/br52/audio_demo/audio_adc_demo.c



c_SRC_FILES += audio/cpu/br52/audio_demo/audio_dac_demo.c audio/cpu/br52/audio_demo/audio_fft_demo.c #audio/cpu/br52/audio_demo/audio_alink_demo.c #audio/cpu/br52/audio_demo/audio_pdm_demo.c
c_SRC_FILES += cpu/br52/setup.c cpu/br52/overlay_code.c





c_SRC_FILES += cpu/br52/charge/charge.c cpu/br52/charge/charge_config.c
c_SRC_FILES += cpu/br52/charge/chargestore.c cpu/br52/charge/chargestore_config.c







c_SRC_FILES += cpu/br52/periph/touch/lpctmu_hw.c cpu/br52/periph/touch/lp_touch_key.c
c_SRC_FILES += cpu/br52/periph/led/pwm_led.c




c_SRC_FILES += cpu/br52/power/power_port.c cpu/br52/power/power_gate.c cpu/br52/power/key_wakeup.c cpu/br52/power/dcvdd_ocp.c cpu/br52/power/power_app.c cpu/br52/power/power_config.c
