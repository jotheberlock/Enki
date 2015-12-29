#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.029.branches_in_function.e
$OUTPUT
expectResult 6

