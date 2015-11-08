#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.018.else.e
$OUTPUT
expectResult 5
