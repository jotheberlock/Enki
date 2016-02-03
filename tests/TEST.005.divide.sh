#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.005.divide.e
$OUTPUT
expectExit 2

