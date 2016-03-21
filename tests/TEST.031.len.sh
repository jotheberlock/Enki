#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.031.len.e
$OUTPUT
expectResult 12
