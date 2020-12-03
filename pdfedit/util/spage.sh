#!/bin/bash
rm -Rf pages
mkdir pages
pdfseparate "$1" pages/page%04d.pdf
for i in pages/*.pdf; do
    ./decompress.sh "$i" || exit 1
    mv "$i.decom.pdf" "$i" || exit 1
done
