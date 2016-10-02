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
echo "Building protobuf 2.5.0"
make
OUT=$?
configfile=./configure
if [ $OUT -eq 0 ] && [ -e "$configfile" ]; then
	echo "protobuf 2.5.0 is ok."
else
	echo "Configuring protobuf 2.5.0"
	chmod +x autogen.sh
	./autogen.sh
	./configure --build=i686-pc-linux-gnu --prefix=/usr CFLAGS="-m32 -DNDEBUG" CXXFLAGS="-m32 -DNDEBUG" LDFLAGS=-m32
	echo "Building protobuf 2.5.0"
	make -j6
fi
