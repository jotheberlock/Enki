#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.054.loaderbug.e
$OUTPUT
expectResult 101

