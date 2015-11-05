#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.024.or.e
$OUTPUT
expectResult 7

