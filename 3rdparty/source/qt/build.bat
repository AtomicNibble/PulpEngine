@echo off
set "qtver=%1"
set "prefix=Qt"
set "suffix=_src"
set "buildSuffix=%2"
set "folder=%prefix%%qtver%%suffix%"
if "%buildSuffix%" == "" set compilefolder=%prefix%%qtver%
if NOT "%buildSuffix%" == "" set compilefolder=%prefix%%qtver%_%buildSuffix%

if "%qtver%" == "" goto :leave



set PATH=%cd%\%folder%\bin;%PATH%
set QTDIR=%cd%\%folder%\qtbase
call cd %compilefolder%

CALL jom -j4
CALL jom install
::CALL nmake clean

cd ..

:leave
if "%qtver%" == "" echo Please enter a version as the first parameter, and the build directory suffix as the second parameter
exit /b 1