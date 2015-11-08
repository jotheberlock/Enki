#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.015.minus.e
$OUTPUT
expectResult 3

