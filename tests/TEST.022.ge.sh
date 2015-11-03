#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.022.ge.e
$OUTPUT
expectResult 1
