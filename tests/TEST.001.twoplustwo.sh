#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.001.twoplustwo.e
$OUTPUT
expectExit 4

