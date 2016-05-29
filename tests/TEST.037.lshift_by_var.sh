#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.037.lshift_by_var.e
$OUTPUT
expectExit 8

