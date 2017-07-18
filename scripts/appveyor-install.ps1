git submodule update --init --recursive
cd c:\github\NoCheatZ-4\server-plugin\SourceSdk\Interfaces\Protobuf\
Start-FileDownload "https://github.com/google/protobuf/archive/v2.5.0.zip"
Expand-Archive v2.5.0.zip -DestinationPath c:\github\NoCheatZ-4\server-plugin\SourceSdk\Interfaces\Protobuf\
del v2.5.0.zip
cd protobuf-2.5.0\
Start-FileDownload "https://github.com/google/googletest/archive/release-1.5.0.zip"
Expand-Archive release-1.5.0.zip -DestinationPath c:\github\NoCheatZ-4\server-plugin\SourceSdk\Interfaces\Protobuf\protobuf-2.5.0\
del release-1.5.0.zip
Rename-Item googletest-release-1.5.0 gtest