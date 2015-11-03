#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.023.ne.e
$OUTPUT
expectResult 1
