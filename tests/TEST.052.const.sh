#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.052.const.e
$OUTPUT
expectResult 42

