#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.009.logicalor.e
$OUTPUT
expectExit 3


