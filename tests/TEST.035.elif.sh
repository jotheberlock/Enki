#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.035.elif.e
$OUTPUT
expectResult 2


