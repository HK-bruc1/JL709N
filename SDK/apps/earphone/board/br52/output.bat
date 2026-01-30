@echo off

set HEADER=apps\earphone\board\br52\customer.h

if exist %HEADER% del %HEADER%

REM Get current timestamp (YYYY-MM-DD HH:MM:SS)
for /f "tokens=1-3 delims=/- " %%a in ("%date%") do (
    set YYYY=%%a
    set MM=%%b
    set DD=%%c
)
set TIME_STR=%time:~0,8%
set TIMESTAMP=%YYYY%-%MM%-%DD% %TIME_STR%

REM Write file header and include guard
echo /* This file is auto-generated. Do not edit manually. */ >> %HEADER%
echo /* Generated at %TIMESTAMP% */ >> %HEADER%
echo. >> %HEADER%
echo #ifndef __CUSTOMER_H__ >> %HEADER%
echo #define __CUSTOMER_H__ >> %HEADER%
echo. >> %HEADER%

REM Generate empty definition for first argument
echo #define _%1 >> %HEADER%
echo. >> %HEADER%

REM Process customer.conf and append macros
powershell -Command "Select-String -Path 'apps\earphone\board\br52\customer.conf' -Pattern '^_.*=' | ForEach-Object { $parts = $_.Line -split '=', 2; '#define ' + $parts[0].Trim() + ' ' + $parts[1].Trim() } | Add-Content '%HEADER%'"

REM Write include guard footer
echo. >> %HEADER%
echo #endif /* __CUSTOMER_H__ */ >> %HEADER%
