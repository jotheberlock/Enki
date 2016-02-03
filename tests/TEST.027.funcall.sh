#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.027.funcall.e
$OUTPUT
expectResult 4

