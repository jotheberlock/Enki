#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.039.rtti.e
STDOUT=`$OUTPUT`
expectResult 0 "$STDOUT" "Foo"
