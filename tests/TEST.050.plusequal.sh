
#!/bin/bash
. common.sh
rm -f $OUTPUT
compile TEST.050.plusequal.e
$OUTPUT
expectResult 5

