#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.009.logicalnot.e
$OUTPUT
expectExit 1

