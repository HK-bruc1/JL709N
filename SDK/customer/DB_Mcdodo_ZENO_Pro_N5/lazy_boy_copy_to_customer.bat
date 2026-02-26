@echo off
set PATH=tools\utils;%PATH%
echo ********************************************************************************
echo Copy config files to client folder - %date%
echo ********************************************************************************

echo Copying files...

copy "..\..\apps\earphone\board\br52\sdk_config.h" . >nul 2>&1 && echo [OK] copy_to_customer sdk_config.h || echo [MISS] copy_to_customer sdk_config.h
copy "..\..\apps\earphone\board\br52\sdk_config.c" . >nul 2>&1 && echo [OK] copy_to_customer sdk_config.c || echo [MISS] copy_to_customer sdk_config.c
copy "..\..\apps\earphone\board\br52\jlstream_node_cfg.h" . >nul 2>&1 && echo [OK] copy_to_customer jlstream_node_cfg.h || echo [MISS] copy_to_customer jlstream_node_cfg.h

@rem The ANC file is replaced manually, so there is no need to obtain it. The latest one is in the client folder.
@rem copy "..\..\cpu\br52\tools\anc_coeff.bin" . >nul 2>&1 && echo [OK] copy_to_customer anc_coeff.bin || echo [MISS] copy_to_customer anc_coeff.bin
@rem copy "..\..\cpu\br52\tools\anc_gains.bin" . >nul 2>&1 && echo [OK] copy_to_customer anc_gains.bin || echo [MISS] copy_to_customer anc_gains.bin

@rem Available when clicking export config in visualization tool
copy "..\..\..\output\cfg_tool.bin" . >nul 2>&1 && echo [OK] copy_to_customer cfg_tool.bin || echo [MISS] copy_to_customer cfg_tool.bin
copy "..\..\..\output\stream.bin" . >nul 2>&1 && echo [OK] copy_to_customer stream.bin || echo [MISS] copy_to_customer stream.bin

@rem After jl_isd.bin is generated, it is returned to the corresponding customer directory as a configuration file. Previously, it was copied to the top-level output and then copied to the earphone during compilation. The new architecture copies the corresponding customer's jl_isd.bin to the earphone during compilation, forming a circular overwrite.
copy "..\..\..\output\jl_isd.bin" . >nul 2>&1 && echo [OK] copy_to_customer jl_isd.bin || echo [MISS] copy_to_customer jl_isd.bin

@rem Tone files need to be exported separately
copy "..\..\..\output\tone_en.cfg" . >nul 2>&1 && echo [OK] copy_to_customer tone_en.cfg || echo [MISS] copy_to_customer tone_en.cfg
copy "..\..\..\output\tone_zh.cfg" . >nul 2>&1 && echo [OK] copy_to_customer tone_zh.cfg || echo [MISS] copy_to_customer tone_zh.cfg

if exist ".\src" rmdir /s /q ".\src"
xcopy "..\..\..\src" ".\src" /y /s /e /i /q >nul 2>&1 && echo [OK] copy_to_customer src directory || echo [MISS] copy_to_customer src directory

echo ********************************************************************************
echo Copy completed!
echo ********************************************************************************
pause