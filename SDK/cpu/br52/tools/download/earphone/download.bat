@echo off
@echo ********************************************************************************
@echo earphone\download.bat %1 %2 %3 %4
@echo %date%
@echo ********************************************************************************

cd %~dp0

copy ..\..\anc_coeff.bin .
copy ..\..\anc_gains.bin .
copy ..\..\br52loader.bin .
copy ..\..\ota.bin .


for /f "tokens=1,2,3* delims=-// " %%i in ('date /t') do set yyyy=%%i&set mm=%%j&set dd=%%k


@echo ----chipkey=%2----


if "%2"=="8134" (
    set CHIPKEY=-key AC690X-8134.key
) else if "%2"=="Nokey" (
    set CHIPKEY=
) else (
    set CHIPKEY=-key 141-AMW-AC690X-41C3.key
)

if not %KEY_FILE_PATH%A==A set KEY_FILE=-key %KEY_FILE_PATH%

REM if %PROJ_DOWNLOAD_PATH%A==A set PROJ_DOWNLOAD_PATH=..\..\..\..\..\..\output
if %PROJ_DOWNLOAD_PATH%A==A set PROJ_DOWNLOAD_PATH=..\..\..\..\..\customer\%1

REM copy %PROJ_DOWNLOAD_PATH%\*.bin .	
REM copy %PROJ_DOWNLOAD_PATH%\*.bin . Copy one by one through the script from %1 to cpu\br52\tools\download\earphone\
REM cfg_tool.bin jl_isd.bin stream.bin
if exist %PROJ_DOWNLOAD_PATH%\tone_en.cfg copy %PROJ_DOWNLOAD_PATH%\tone_en.cfg .	
if exist %PROJ_DOWNLOAD_PATH%\tone_zh.cfg copy %PROJ_DOWNLOAD_PATH%\tone_zh.cfg .
if exist sdk_config.h del sdk_config.h
if exist sdk_config.c del sdk_config.c

if %TONE_EN_ENABLE%A==1A (
    if not exist tone_en.cfg copy ..\..\tone.cfg tone_en.cfg
    set TONE_FILES=tone_en.cfg
)
if %TONE_ZH_ENABLE%A==1A (
    set TONE_FILES=%TONE_FILES% tone_zh.cfg
)

if %FORMAT_VM_ENABLE%A==1A set FORMAT=-format vm
if %FORMAT_ALL_ENABLE%A==1A set FORMAT=-format all

if not %RCSP_EN%A==A (
   ..\..\json_to_res.exe ..\..\json.txt
    set CONFIG_DATA=config.dat
)


@echo on
..\..\isd_download.exe ..\..\isd_config.ini -tonorflash -dev br52 -boot 0x102600 -div8 -wait 300 -uboot ..\..\uboot.boot -app ..\..\app.bin  -tone %TONE_FILES% -res ALIGN_DIR cfg_tool.bin ..\..\p11_code.bin stream.bin %CONFIG_DATA% %KEY_FILE% %FORMAT% -output-ufw update.ufw %CHIPKEY%
@echo off
:: -format all
::-reboot 2500

@rem 删除临时文件-format all
if exist *.mp3 del *.mp3 
if exist *.PIX del *.PIX
if exist *.TAB del *.TAB
if exist *.res del *.res
if exist *.sty del *.sty

@rem 生成固件升级文件
::..\..\fw_add.exe -noenc -fw jl_isd.fw -add ..\..\ota.bin -type 100 -out jl_isd.fw
@rem 添加配置脚本的版本信息到 FW 文件中
::..\..\fw_add.exe -noenc -fw jl_isd.fw -add ..\..\script.ver -out jl_isd.fw

::..\..\ufw_maker.exe -fw_to_ufw jl_isd.fw
::copy jl_isd.ufw update.ufw
::del jl_isd.ufw

set GIT_VER_EX=%4
set GIT_VER=%GIT_VER_EX:~0, 9%
set dirPath=..\..\..\..\..\output\%1_%3_git_%GIT_VER%_%yyyy%_%mm%_%dd%

mkdir "!dirPath!" 

REM copy update.ufw %PROJ_DOWNLOAD_PATH%\update.ufw
copy update.ufw %dirPath%\%1_update.ufw
@rem After jl_isd.bin is generated, it is returned to the corresponding customer directory as a configuration file. Previously, it was copied to the top-level output and then copied to the earphone during compilation. The new architecture copies the corresponding customer's jl_isd.bin to the earphone during compilation, forming a circular overwrite.
copy jl_isd.bin %PROJ_DOWNLOAD_PATH%\jl_isd.bin

REM copy jl_isd.fw %PROJ_DOWNLOAD_PATH%\jl_isd.fw
copy jl_isd.fw %dirPath%\%1_jl_isd.fw

REM To ensure consistent and aesthetically pleasing firmware names
copy update.ufw ..\..\..\..\..\..\output\%1_update.ufw
copy jl_isd.fw ..\..\..\..\..\..\output\%1_jl_isd.fw
copy jl_isd.bin ..\..\..\..\..\..\output\jl_isd.bin


if %UPDATE_COMPRESS_ENABLE%A==1A (
    ..\..\isd_download.exe -make-upgrade-bin -ufw update.ufw -output db_update.bin
    ..\..\ufw_maker.exe -chip JL709N -enc -res db_update.bin -output update-com.ufw
    copy update-com.ufw %PROJ_DOWNLOAD_PATH%\update-com.ufw
)

@rem 常用命令说明
@rem -format vm        //擦除VM 区域
@rem -format cfg       //擦除BT CFG 区域
@rem -format 0x3f0-2   //表示从第 0x3f0 个 sector 开始连续擦除 2 个 sector(第一个参数为16进制或10进制都可，第二个参数必须是10进制)

ping /n 2 127.1>null
IF EXIST null del null
