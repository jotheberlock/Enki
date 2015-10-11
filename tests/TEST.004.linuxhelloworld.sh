#!/bin/bash
. common.sh
linuxonly
rm -f $OUTPUT
compile TEST.004.linuxhelloworld.e
STDOUT=`$OUTPUT`
expectExit 12
expectString "$STDOUT" "Hello world\n"


