

REM build the assets..

SEt WWISE_BIN=C:\Program Files (x86)\Audiokinetic\Wwise v2015.1.5 build 5533\Authoring\x64\Release\bin

set BIN_PATH=..\..\build\x64\Super
set REL_PATH="../../build/x64/Release Dynamic"
set ABS_PATH=

pushd %REL_PATH%
set ABS_PATH=%CD%
popd

REM convert all the assets...
REM "%ABS_PATH%\engine_Converter.exe" +mode all +mod core +profile release +nopause

REM build all shader perms...
"%ABS_PATH%\engine_ShaderCompiler.exe" +mode all +nopause

REM build levels...
REM "%ABS_PATH%\engine_LevelBuilder.exe" +if art_source\maps\test01.map +mod core +nopause

REM pack them up!
"%ABS_PATH%\engine_Linker.exe" +mode build +of core_assets\core +nopause +nocompress +mod core

REM build sound packs...
REM "%WWISE_BIN%\WwiseCLI.exe" "art_source\sound\wwise\Projects\Potato\Potato.wproj" -GenerateSoundBanks -Platform Windows -Verbose

mkdir build
mkdir build\core_assets

rem Copy binary..

xcopy /y /d "%BIN_PATH%\PhysX*.dll" "build"
xcopy /y /d "%BIN_PATH%\nvToolsExt*.dll" "build"
xcopy /y /d "%BIN_PATH%\engine_Game.exe" "build"

rem 3rd party dlls
xcopy /y /d "..\3rdparty\bin\x64\D3DCompiler_47.dll" "build"

rem some assets.. !
xcopy /y /d "core_assets\*.apak" "build\core_assets"
xcopy /y /i /s /e /d "core_assets\config" "build\core_assets\config"
xcopy /y /i /s /e /d "core_assets\levels" "build\core_assets\levels"
xcopy /y /i /s /e /d "core_assets\sound" "build\core_assets\sound"
xcopy /y /i /s /e /d "core_assets\techdefs" "build\core_assets\techdefs"
xcopy /y /i /s /e /d "core_assets\shaders" "build\core_assets\shaders"
xcopy /y /i /s /e /d "core_assets\scripts" "build\core_assets\scripts"

del /s /q /f "build\core_assets\sound\*.xml"
del /s /q /f "build\core_assets\sound\*.json"
del /s /q /f "build\core_assets\sound\*.txt"

rmdir /s /q "build\core_assets\shaders\old"
rmdir /s /q "build\core_assets\shaders\temp"

del /s /q game.7z

"C:\Program Files\7-Zip\7z.exe" a -r -m0=lzma2 -mmt=6 -mx=2 game "./build/*"

rmdir /s /q build