#!/bin/bash
list=`ls pages/*.pdf | sort`
if [ -z "$list" ]; then echo Nothing; exit 1; fi
pdfunite $list __recombined__.pdf

./sanitize.sh __recombined__.pdf
./compress.sh __recombined__.pdf.san.pdf

