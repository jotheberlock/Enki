#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.045.pointers_in_structs.e
STDOUT=`$OUTPUT`
expectResult 6 "$STDOUT" "World"

