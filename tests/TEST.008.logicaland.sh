#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.008.logicaland.e
$OUTPUT
expectExit 2

