#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.046.generics_with_differing_numbers_of_arguments.e
STDOUT=`$OUTPUT`
expectResult 0 "$STDOUT" "HelloHi"

