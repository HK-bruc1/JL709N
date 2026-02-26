@echo off
@echo ********************************************************************************
@echo SDK BR52
@echo %date%
@echo ********************************************************************************

cd %~dp0

set PATH=tools\utils;%PATH%

if exist customer_path.conf del customer_path.conf

set para=%1

@rem  ###-customer + blname or model
@rem  ###-DB47
@rem  ###-DB_Mcdodo_ZENO_Pro_N5
set  CUSTOMER_PATH=DB_Mcdodo_ZENO_Pro_N5
echo CUSTOMER_PATH=%CUSTOMER_PATH%>customer_path.conf
@rem ###  key select
@rem ###  8134: AC690X-8134.key
@rem ###  41C3: 141-AMW-AC690X-41C3.key
@rem ###  Nokey: 
set CUSTOMER_CHIPKEY=41C3
echo CUSTOMER_CHIPKEY=%CUSTOMER_CHIPKEY%>>customer_path.conf

@rem Retrieve configuration from the corresponding customer folder
call ".\customer_copy.bat"  %CUSTOMER_PATH%
@rem Convert the conf file to customer.h file
call ".\apps\earphone\board\br52\output.bat"  %CUSTOMER_PATH%

if "%para%"=="" set para=all
if /i "%para%"=="all" (
    make all -j %NUMBER_OF_PROCESSORS%
) else if /i "%para%"=="clean" (
    make clean
) else (
    @echo Invalid parameter: %para%. Usage: all ^| clean && exit /b 1
)
if errorlevel 1 (@echo Build failed! && exit /b 1)
@echo Build completed: %para%.
