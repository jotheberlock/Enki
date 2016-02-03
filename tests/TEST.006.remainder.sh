#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.006.remainder.e
$OUTPUT
expectExit 2

