#!/bin/bash
. common.sh
rm -f $OUTPUT

if [[ `uname -m` == "armv7l" ]]; then
    compile TEST.034.num_to_str.e
    STDOUT=`$OUTPUT`
    expectResult 16 "$STDOUT" "        deadbeef"
else
    compile TEST.034.num_to_str.e
    STDOUT=`$OUTPUT`
    expectResult 16 "$STDOUT" "$STDOUT"
fi
