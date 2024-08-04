#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.055.logicalxor.e
$OUTPUT
expectExit 6
