#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.016.while.e
$OUTPUT
expectResult 12

