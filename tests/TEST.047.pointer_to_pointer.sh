#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.047.pointer_to_pointer.e
STDOUT=`$OUTPUT`
expectResult 6 "$STDOUT" "Hello"
