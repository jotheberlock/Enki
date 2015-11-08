#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.025.xor.e
$OUTPUT
expectResult 3

