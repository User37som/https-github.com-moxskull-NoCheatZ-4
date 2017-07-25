mkdir ..\Builds\%1\cfg
mkdir ..\Builds\%1\addons\NoCheatZ
mkdir ..\Builds\%1\logs\NoCheatZ_4_Logs
mkdir ..\Builds\%1\NoCheatZ-autorecords
xcopy ..\.gitignore ..\Builds\%1\logs\NoCheatZ_4_Logs /F /Y
xcopy ..\.gitignore ..\Builds\%1\NoCheatZ-autorecords /F /Y
xcopy ..\server-plugin\Res\nocheatz.cfg ..\Builds\%1\cfg /F /Y
xcopy ..\server-plugin\Res\nocheatz.vdf ..\Builds\%1\addons\ /F /Y
xcopy ..\server-plugin\Res\config.ini ..\Builds\%1\addons\NoCheatZ /F /Y
