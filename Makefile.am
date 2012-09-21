# Makefile for profile collector package
# $Id: Makefile 12318 2005-12-31 15:34:46Z midom $
#
bin_PROGRAMS = collector filter
collector_SOURCES = src/collector.c src/collector.h src/export.c src/export.h
filter_SOURCES = src/filter.c src/filter.h

if DEBUG
	CFLAGS = -g3 -O0 -Wall -pedantic
	CPPFLAGS = -g3 -O0 -Wall -pedantic
else
	CFLAGS = -O3 $AM_LDFLAGS
	CPPFLAGS = -O3 $AM_LDFLAGS
endif


LIBS = $(DEPS_LIBS)
INCLUDES = $(DEPS_CFLAGS)
AM_CPPFLAGS = $(DEPS_CFLAGS)

#ACLOCAL_AMFLAGS = -I m4


all: collector filter

#collector_LDADD = $(DEPS_LIBS) -ldb -lpthread 
collector: src/collector.h src/collector.c src/export.c export.o
	cc -o collector src/collector.c export.o -ldb -lpthread


filter: src/filter.c
	cc -o filter src/filter.c


export.o: src/collector.h src/export.c
	cc -c -o export.o src/export.c

clean:
	rm -f collector exporter filter

