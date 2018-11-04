

REM build the assets..

SEt WWISE_BIN=C:\Program Files (x86)\Audiokinetic\Wwise v2015.1.5 build 5533\Authoring\x64\Release\bin

SET MAKE_ZIP=

IF defined MAKE_ZIP (
set OUT_PATH=build
) ELSE (
set OUT_PATH=../../public_build/game
)

set GAME_BIN_PATH=../../build/x64/Super
set BIN_PATH_REL="../../build/x64/Debug Dynamic"
set BIN_PATH_ABS=
set OUT_PATH_ABS=

set LINKER_ARGS= +mod core +nopause +nocompress

pushd %BIN_PATH_REL%
set BIN_PATH_ABS=%CD%
popd

CALL :NORMALIZEPATH "%OUT_PATH%"
SET OUT_PATH_ABS=%RETVAL%


IF NOT defined MAKE_ZIP (
REM clean..
rmdir /s /q "%OUT_PATH_ABS%"
)

mkdir "%OUT_PATH_ABS%"
mkdir "%OUT_PATH_ABS%\core_assets"

REM convert all the assets...
"%BIN_PATH_ABS%\engine_Converter.exe" +mode all +mod core +profile release +nopause


REM build all shader perms...
"%BIN_PATH_ABS%\engine_ShaderCompiler.exe" +mode all +nopause

REM build levels...
"%BIN_PATH_ABS%\engine_LevelBuilder.exe" +if art_source\maps\test01.map +mod core +nopause
"%BIN_PATH_ABS%\engine_LevelBuilder.exe" +if art_source\maps\text_cord_test.map +mod core +nopause

REM Base packs
"%BIN_PATH_ABS%\engine_Linker.exe" +mode build +of %OUT_PATH_ABS%\core_assets\init %LINKER_ARGS% +al asset_lists/init.assList +memory

"%BIN_PATH_ABS%\engine_Linker.exe" +mode build +of %OUT_PATH_ABS%\core_assets\weapons %LINKER_ARGS% +al asset_lists/weapons.assList +memory

"%BIN_PATH_ABS%\engine_Linker.exe" +mode build +of %OUT_PATH_ABS%\core_assets\shaders %LINKER_ARGS% +al asset_lists/shaders.assList +memory

REM level packs
"%BIN_PATH_ABS%\engine_Linker.exe" +mode build +of %OUT_PATH_ABS%\core_assets\test01 %LINKER_ARGS% +lvl test01
"%BIN_PATH_ABS%\engine_Linker.exe" +mode build +of %OUT_PATH_ABS%\core_assets\text_cord_test %LINKER_ARGS% +lvl text_cord_test

REM build sound packs...
"%WWISE_BIN%\WwiseCLI.exe" "art_source\sound\wwise\Projects\Potato\Potato.wproj" -GenerateSoundBanks -Platform Windows -Verbose

rem Copy binary..
xcopy /y /d "%GAME_BIN_PATH%\engine_Game.exe" "%OUT_PATH%"

rem 3rd party dlls
xcopy /y /d "..\3rdparty\Physx_libs\bin\x64\PhysX.dll" "%OUT_PATH%"
xcopy /y /d "..\3rdparty\Physx_libs\bin\x64\PhysXCommon.dll" "%OUT_PATH%"
xcopy /y /d "..\3rdparty\Physx_libs\bin\x64\PhysXCooking.dll" "%OUT_PATH%"
xcopy /y /d "..\3rdparty\Physx_libs\bin\x64\PhysXCharacterKinematic.dll" "%OUT_PATH%"

rem copy all physics for now, be while before have actual release builds.
xcopy /y /d "..\3rdparty\Physx_libs\bin\x64\PhysX*.dll" "%OUT_PATH%"

xcopy /y /d "..\3rdparty\Physx_libs\bin\x64\nvToolsExt*.dll" "%OUT_PATH%"

xcopy /y /d "..\3rdparty\bin\x64\D3DCompiler_47.dll" "%OUT_PATH%"

rem for debug builds..
xcopy /y /d "..\3rdparty\bin\x64\ispc_texcomp.dll" "%OUT_PATH%"


rem some assets.. !
REM xcopy /y /d "core_assets\*.apak" "%OUT_PATH%\core_assets"
xcopy /S /Q /Y /F "core_assets\config\default.cfg" "%OUT_PATH%\core_assets\config\"
REM xcopy /y /i /s /e /d "core_assets\levels" "%OUT_PATH%\core_assets\levels"
xcopy /y /i /s /e /d "core_assets\sound" "%OUT_PATH%\core_assets\sound"
REM xcopy /y /i /s /e /d "core_assets\techdefs" "%OUT_PATH%\core_assets\techdefs"
REM xcopy /y /i /s /e /d "core_assets\shaders" "%OUT_PATH%\core_assets\shaders"
REM xcopy /y /i /s /e /d "core_assets\scripts" "%OUT_PATH%\core_assets\scripts"
REM xcopy /y /i /s /e /d "core_assets\menus" "%OUT_PATH%\core_assets\menus"

del /s /q /f "%OUT_PATH%\core_assets\sound\*.xml"
del /s /q /f "%OUT_PATH%\core_assets\sound\*.json"
del /s /q /f "%OUT_PATH%\core_assets\sound\*.txt"

REM rmdir /s /q "%OUT_PATH%\core_assets\shaders\old"
REM rmdir /s /q "%OUT_PATH%\core_assets\shaders\temp"

del /s /q game.7z

IF defined MAKE_ZIP (

"C:\Program Files\7-Zip\7z.exe" a -r -m0=lzma2 -mmt=6 -mx=2 game "./build/*"

rmdir /s /q build
) ELSE (

cd "%OUT_PATH%"
git add .
git commit -m "Update Build"
git push
)


EXIT /B

:NORMALIZEPATH
  SET RETVAL=%~dpfn1
  EXIT /B