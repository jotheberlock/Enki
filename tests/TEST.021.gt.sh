#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.021.gt.e
$OUTPUT
expectResult 1
