#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.053.elif2.e
STDOUT=`$OUTPUT`
expectResult 0 "$STDOUT" "ZeroOneTwoThree"



