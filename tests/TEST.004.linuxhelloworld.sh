#!/bin/bash
. common.sh
linuxonly
amd64only
rm -f $OUTPUT
compile TEST.004.linuxhelloworld.e
STDOUT=`$OUTPUT`
expectResult 12 "$STDOUT" "Hello world"


