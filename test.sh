#!/bin/sh
cd encode;
make
cd ..
./encode/encode $1 tmp
./decode/0016326_hw2_de tmp 123
diff 123 $1
rm 123 tmp -f
