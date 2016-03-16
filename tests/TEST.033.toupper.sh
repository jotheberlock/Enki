#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.033.toupper.e
echo "Hello" | ./$OUTPUT > stdout.txt
STDOUT=`cat stdout.txt`
# 0 because cat gets run after the test program
expectResult 0 $STDOUT "HELLO" 
