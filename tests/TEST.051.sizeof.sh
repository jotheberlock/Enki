#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.051.sizeof.e
$OUTPUT
expectResult 4

