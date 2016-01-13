#!/bin/bash
. common.sh
windowsonly
rm -f $OUTPUT
compile TEST.030.windowshelloworld.e
STDOUT=`$OUTPUT`
expectResult 12 "$STDOUT" "Hello world"
