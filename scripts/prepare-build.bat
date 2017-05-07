mkdir ..\Builds\%1\cfg
mkdir ..\Builds\%1\addons\NoCheatZ
mkdir ..\Builds\%1\logs\NoCheatZ_4_Logs
mkdir ..\Builds\%1\NoCheatZ-autorecords
xcopy ..\.gitignore ..\Builds\%1\logs\NoCheatZ_4_Logs /F /Y
xcopy ..\.gitignore ..\Builds\%1\NoCheatZ-autorecords /F /Y
xcopy ..\server-plugin\Res\nocheatz.cfg ..\Builds\%1\cfg /F /Y
xcopy ..\server-plugin\Res\nocheatz.vdf ..\Builds\%1\addons\ /F /Y
xcopy ..\server-plugin\Res\config.ini ..\Builds\%1\addons\NoCheatZ /F /Y

del Code\version.cpp
git describe --tags --always --dirty > gitvtmp.txt
git describe --tags --abbrev=0 > gitvshorttmp.txt
set /p gitvout=<gitvtmp.txt
set /p gitvshortout=<gitvshorttmp.txt
(
echo char const * const NCZ_VERSION_GIT = ^"%gitvout%%2^";
echo char const * const NCZ_VERSION_GIT_SHORT = ^"%gitvshortout%^";
) > Code\version.cpp
del gitvtmp.txt
del gitvshorttmp.txt
