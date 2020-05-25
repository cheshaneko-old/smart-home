#/bin/bash
echo "//Auto generated from wifichoose.html with " $0 > Page.h
echo -n "byte htmlPage[] = {" >> Page.h
gzip -v -c wifichoose.html |
hexdump -v -e '"0x" 1/1 "%02X" ", "' |  sed 's/.$//' |  sed 's/.$//' >> Page.h
echo "};" >> Page.h

