#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.032.write.e
STDOUT=`$OUTPUT`
expectResult 12 "$STDOUT" "Hello world"
