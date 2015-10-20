#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.014.array.e
$OUTPUT
expectExit 4
