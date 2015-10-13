#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.007.sub.e
$OUTPUT
expectExit 3

