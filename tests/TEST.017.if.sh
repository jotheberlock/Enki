#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.017.if.e
$OUTPUT
expectResult 3
