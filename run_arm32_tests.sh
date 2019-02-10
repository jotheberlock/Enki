#!/bin/bash
export ENKI_NATIVE_TARGET=linux_arm32_target.ini
export TERM=vt220
cd ~/Enki
git pull
make clean
make
cd tests
./runTests.sh
