#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.034.num_to_str.e
STDOUT=`$OUTPUT`
expectResult 16 "$STDOUT" "deadbeefdeadbeef"
