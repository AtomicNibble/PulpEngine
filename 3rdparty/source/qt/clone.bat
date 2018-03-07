@echo off
set "qtver=%1"
set "prefix=Qt"
set "suffix=_src"
set "folder=%prefix%%qtver%%suffix%"

if "%qtver%" == "" goto :leave


CALL git clone https://code.qt.io/qt/qt5.git %folder%
CALL cd %folder%
CALL git checkout .
CALL git checkout %qtver%
if NOT %ERRORLEVEL% == 0 goto :errorcheckout
CALL perl init-repository
CALL cd ..\

echo Download done...
pause


:leave
if "%qtver%" == "" echo Please enter a version as the first parameter, and the build directory suffix as the second parameter
exit /b 1