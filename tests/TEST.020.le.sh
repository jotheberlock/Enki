#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.020.le.e
$OUTPUT
expectResult 3
