#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.042.generics_with_basic_types.e
STDOUT=`$OUTPUT`
expectResult 3 "$STDOUT" "Uint,Uint Uint,Byte^ Byte^,Uint "

