# Makefile for profile collector package
# $Id: Makefile 12318 2005-12-31 15:34:46Z midom $
# remove the topbuildir
bin_PROGRAMS = collector filter 
collector_SOURCES = src/collector.c src/collector.h src/export.c src/export.h
filter_SOURCES = src/filter.c src/filter.h



if DEBUG
       AM_CFLAGS = -g3 -O0 -Wall -pedantic $(DEPS_CFLAGS) $(AM_LDFLAGS)
       AM_CPPFLAGS = -g3 -O0 -Wall -pedantic $(DEPS_CPPFLAGS) $(AM_LDFLAGS)
else
       AM_CFLAGS = -O2 $(DEPS_CFLAGS) $(AM_LDFLAGS)
       AM_CPPFLAGS = -O2 $(DEPS_CPPFLAGS) $(AM_LDFLAGS)
endif



CC=gcc $(AM_CFLAGS)


LIBS = $(DEPS_LIBS)
INCLUDES = $(DEPS_CFLAGS)

ACLOCAL_AMFLAGS = -I m4


all: collector filter

collector: src/collector.h src/collector.c src/export.c export.o
	$(CC) -o collector src/collector.c export.o -ldb -lpthread


filter: src/filter.c
	$(CC) -o filter src/filter.c


export.o: src/collector.h src/export.c
	$(CC) -c -o export.o src/export.c

clean:
	rm -f collector export.o filter

