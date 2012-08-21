#! /bin/sh

make clean
rm Makefile
rm Makefile.in
rm config.status
rm aclocal.m4
rm -rf autom4te.cache
rm configure

aclocal \
&& automake --gnu --add-missing \
&& autoconf
autoreconf -i
./configure
make

#cat  ~/Development/analytics/udp-filters/data/sampled-1000.log-20111224 | ./filter 

