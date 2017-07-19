$ErrorActionPreference = 'SilentlyContinue'
cd ..\Builds
Get-ChildItem -Include *.exp | Remove-Item -recurse
Get-ChildItem -Include *.ilk | Remove-Item -recurse 
Get-ChildItem -Include *.lib | Remove-Item -recurse
Get-ChildItem -Include *.bsc | Remove-Item -recurse
Get-ChildItem -Include .gitignore | Remove-Item -recurse
New-Item -ItemType directory -Path out
Compress-Archive -Path Debug\* -DestinationPath out\Debug-Windows.zip
Compress-Archive -Path Release\* -DestinationPath out\Release-Windows.zip
