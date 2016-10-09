#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.041.recursion.e
STDOUT=`$OUTPUT`
expectResult 0 "$STDOUT" "              05              04              03              02              01"
