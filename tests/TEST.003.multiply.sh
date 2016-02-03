#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.003.multiply.e
$OUTPUT
expectExit 10

