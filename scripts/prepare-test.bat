E:\steamcmd\steamcmd.exe +login anonymous +force_install_dir .\%1\ +app_update %3 +quit
xcopy "..\Builds\%2" "E:\steamcmd\%1\%4" /E /F /Y 
