#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.001.twoplustwo.e
$OUTPUT
expectResult 4

