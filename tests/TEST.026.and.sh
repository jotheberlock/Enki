#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.026.and.e
$OUTPUT
expectResult 4
