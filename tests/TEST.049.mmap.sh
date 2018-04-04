#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.049.mmap.e
STDOUT=`$OUTPUT`
expectResult 12 "$STDOUT" "Hello World"
