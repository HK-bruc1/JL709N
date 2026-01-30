@echo off
@echo ********************************************************************************
@echo customer_copy.bat %1
@echo %date%
@echo ********************************************************************************
set CUSTOMER_PATH=%1

copy .\customer\%CUSTOMER_PATH%\customer.conf .\apps\earphone\board\br52 >nul 2>&1 && echo [OK] copy_to_sdk customer.conf || echo [MISS] copy_to_sdk customer.conf
copy .\customer\%CUSTOMER_PATH%\sdk_config.h .\apps\earphone\board\br52 >nul 2>&1 && echo [OK] copy_to_sdk sdk_config.h || echo [MISS] copy_to_sdk sdk_config.h
copy .\customer\%CUSTOMER_PATH%\sdk_config.c .\apps\earphone\board\br52 >nul 2>&1 && echo [OK] copy_to_sdk sdk_config.c || echo [MISS] copy_to_sdk sdk_config.c
copy .\customer\%CUSTOMER_PATH%\jlstream_node_cfg.h .\apps\earphone\board\br52 >nul 2>&1 && echo [OK] copy_to_sdk jlstream_node_cfg.h || echo [MISS] copy_to_sdk jlstream_node_cfg.h

@rem Copy the Google GFPS service ID, along with the public and private key credentials, from the customer directory to the specific application directory.
copy .\customer\%CUSTOMER_PATH%\google_gfps_config.h .\apps\common\third_party_profile\gfps_protocol\google_gfps_config.h >nul 2>&1 && echo [OK] copy_to_sdk google_gfps_config.h || echo [MISS] copy_to_sdk google_gfps_config.h

@rem These two are temporarily uncertain if they are like this, but they should be like this. In cpu\br52\tools\download\earphone\download.bat, these two files are copied to SDK\cpu\br52\tools\download\earphone
@rem So I only need to ensure that cpu\br52\tools are the customer's ANC-specific files.
copy .\customer\%CUSTOMER_PATH%\anc_coeff.bin .\cpu\br52\tools >nul 2>&1 && echo [OK] copy_to_sdk anc_coeff.bin || echo [MISS] copy_to_sdk anc_coeff.bin
copy .\customer\%CUSTOMER_PATH%\anc_gains.bin .\cpu\br52\tools >nul 2>&1 && echo [OK] copy_to_sdk anc_gains.bin || echo [MISS] copy_to_sdk anc_gains.bin

@rem Important credentials used by the APP to identify different headphone users
copy .\customer\%CUSTOMER_PATH%\json.txt .\cpu\br52\tools >nul 2>&1 && echo [OK] copy_to_sdk json.txt || echo [MISS] copy_to_sdk json.txt

@rem After jl_isd.bin is generated, it is returned to the corresponding customer directory as a configuration file. Previously, it was copied to the top-level output and then copied to the earphone during compilation. The new architecture copies the corresponding customer's jl_isd.bin to the earphone during compilation, forming a circular overwrite.
copy .\customer\%CUSTOMER_PATH%\jl_isd.bin .\cpu\br52\tools\download\earphone >nul 2>&1 && echo [OK] copy_to_sdk jl_isd.bin || echo [MISS] copy_to_sdk jl_isd.bin

copy .\customer\%CUSTOMER_PATH%\cfg_tool.bin .\cpu\br52\tools\download\earphone >nul 2>&1 && echo [OK] copy_to_sdk cfg_tool.bin || echo [MISS] copy_to_sdk cfg_tool.bin
@rem Didn't copy this over, which caused the microphone to be configured but still unable to collect sound during calls. This indicates that this file takes effect here.
copy .\customer\%CUSTOMER_PATH%\stream.bin .\cpu\br52\tools\download\earphone >nul 2>&1 && echo [OK] copy_to_sdk stream.bin || echo [MISS] copy_to_sdk stream.bin

xcopy .\customer\%CUSTOMER_PATH%\src  ..\src  /E /Y /I >nul 2>&1 && echo [OK] copy_to_sdk src directory || echo [MISS] copy_to_sdk src directory

@rem Ensure that all places in the project are configured by the customer
copy .\customer\%CUSTOMER_PATH%\cfg_tool.bin ..\output\ >nul 2>&1 && echo [OK] Restore cfg_tool.bin || echo [MISS] Restore cfg_tool.bin
copy .\customer\%CUSTOMER_PATH%\stream.bin ..\output\ >nul 2>&1 && echo [OK] Restore stream.bin || echo [MISS] Restore stream.bin
copy .\customer\%CUSTOMER_PATH%\jl_isd.bin ..\output\ >nul 2>&1 && echo [OK] Restore jl_isd.bin || echo [MISS] Restore jl_isd.bin
copy .\customer\%CUSTOMER_PATH%\tone_en.cfg ..\output\ >nul 2>&1 && echo [OK] Restore tone_en.cfg || echo [MISS] Restore tone_en.cfg
copy .\customer\%CUSTOMER_PATH%\tone_zh.cfg ..\output\ >nul 2>&1 && echo [OK] Restore tone_zh.cfg || echo [MISS] Restore tone_zh.cfg

