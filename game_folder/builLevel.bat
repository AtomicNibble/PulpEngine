REM build the assets..

set GAME_BIN_PATH=..\..\build\x64\Debug
set REL_PATH="../../build/x64/Debug Dynamic"
set ABS_PATH=

pushd %REL_PATH%
set ABS_PATH=%CD%
popd


REM build levels...
"%ABS_PATH%\engine_LevelBuilder.exe" +if art_source\maps\text_cord_test.map +mod core +nopause
