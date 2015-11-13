#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.011.pointer.e
$OUTPUT
expectExit 4

