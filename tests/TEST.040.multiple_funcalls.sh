#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.040.multiple_funcalls.e
STDOUT=`$OUTPUT`
expectResult 0 "$STDOUT" "Foo"
