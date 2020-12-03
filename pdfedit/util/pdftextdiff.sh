#!/bin/bash
pdftotext "$1" temp1.txt || exit 1
pdftotext "$2" temp2.txt || exit 1
diff -u temp1.txt temp2.txt | less
rm -f temp1.txt temp2.txt

