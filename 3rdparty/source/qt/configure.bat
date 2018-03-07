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
mkdir %compilefolder%
call cd %compilefolder%

set platform=win32-msvc2015
::set QMAKESPEC=win32-msvc2015
set QMAKESPEC=
set QMAKEPATH=

set CL=/MP
CALL ..\%folder%\configure -confirm-license -debug-and-release -opensource -platform %platform% -opengl desktop -static -nomake examples -nomake tests -mp -qt-zlib -qt-pcre -qt-libpng -qt-libjpeg -direct2d -no-gif -skip qtconnectivity -skip qtgamepad -skip qtsensors -skip qtserialbus -skip qtlocation -skip qtandroidextras -skip qtwebengine -skip qtsvg -skip qtserialport -skip qtnetworkauth -skip qtx11extras -skip qtwebsockets -skip qtspeech -skip qtscript -skip qtwebview -skip qtwayland -skip  qttools -skip qtactiveqt


cd ..

:leave
if "%qtver%" == "" echo Please enter a version as the first parameter, and the build directory suffix as the second parameter
exit /b 1