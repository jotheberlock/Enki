#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.012.lshift.e
$OUTPUT
expectExit 4
