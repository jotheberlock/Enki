#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.019.lt.e
$OUTPUT
expectResult 1
