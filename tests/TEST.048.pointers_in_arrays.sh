#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.048.pointers_in_arrays.e
STDOUT=`$OUTPUT`
expectResult 6 "$STDOUT" "World"
