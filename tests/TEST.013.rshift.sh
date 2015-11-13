#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.013.rshift.e
$OUTPUT
expectExit 4
