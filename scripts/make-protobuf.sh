#!/bin/bash

cd ./SourceSdk/Interfaces/Protobuf/protobuf-2.5.0
autogenfile=./autogen.sh
if [ ! -e "$autogenfile" ]; then
	cd ..
	echo "Downloading protobuf 2.5.0"
	wget https://github.com/google/protobuf/archive/v2.5.0.tar.gz
	tar xzvf v2.5.0.tar.gz
	rm v2.5.0.tar.gz
	cd protobuf-2.5.0
	echo "Downloading gtest 1.5.0"
	wget https://github.com/google/googletest/archive/release-1.5.0.tar.gz
	tar xzvf release-1.5.0.tar.gz
	rm release-1.5.0.tar.gz
	mv googletest-release-1.5.0 gtest
fi
