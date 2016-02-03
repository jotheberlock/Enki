#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.010.logicalnot.e
$OUTPUT
expectExit 1

