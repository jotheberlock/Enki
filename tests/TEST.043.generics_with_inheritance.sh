#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.043.generics_with_inheritance.e
STDOUT=`$OUTPUT`
expectResult 2 "$STDOUT" "Foo Bar Bar "

