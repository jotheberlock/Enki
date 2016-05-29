#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.038.rshift_by_var.e
$OUTPUT
expectExit 2
