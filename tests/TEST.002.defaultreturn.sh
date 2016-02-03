#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.002.defaultreturn.e
$OUTPUT
expectExit 4

