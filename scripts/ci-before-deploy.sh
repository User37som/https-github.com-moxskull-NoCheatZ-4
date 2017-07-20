#!/bin/bash

cd Builds
rm -rf .gitignore
mkdir out
cd Debug
tar czvf ../out/Debug-Linux.tar.gz *
cd ../Release
tar czvf ../out/Release-Linux.tar.gz *
