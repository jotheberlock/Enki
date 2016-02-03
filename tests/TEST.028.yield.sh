#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.028.yield.e
$OUTPUT
expectResult 6

