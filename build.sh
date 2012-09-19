#!/bin/bash
make clean
#rm Makefile
#rm Makefile.in
#rm config.status
#rm aclocal.m4
#rm -rf autom4te.cache
#rm configure

DUMPS_DIR="dumps"

if [ ! -d "./$DUMPS_DIR" ]; then
  echo -e "Creating dumps/ directory because it was not found...\n"
  mkdir $DUMPS_DIR;
fi


libtoolize --force # generate  ltmain.sh link etc..
autoheader # generating config.in.h
aclocal  
automake --add-missing
autoconf
./configure
make

#cat  ~/Development/analytics/udp-filters/data/sampled-1000.log-20111224 | ./filter 

