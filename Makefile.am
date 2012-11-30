# Makefile for profile collector package
# $Id: Makefile 12318 2005-12-31 15:34:46Z midom $
# remove the topbuildir
bin_PROGRAMS = 	    collector filter collector-static filter-static
collector_SOURCES = src/collector.c src/collector.h src/export.c src/export.h
filter_SOURCES =    src/filter.c src/filter.h

collector_static_SOURCES = $(collector_SOURCES)
filter_static_SOURCES	 = $(filter_SOURCES)

collector_LDADD = -ldb -lpthread
filter_LDADD	= -ldb -lpthread

collector_static_LD_ADD  = $(collector_LDADD)
filter_static_LD_ADD 	 = $(filter_LDADD)



collector_static_LDFLAGS = -all-static
filter_static_LDFLAGS = -all-static

PACKAGE_STRING=webstatscollector 0.2.32
PACKAGE_VERSION=0.2.32
VERSION=0.2.32


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


export.o: src/collector.h src/export.c
	$(CC) -c -o export.o src/export.c

clean:
	rm -f filter.o collector.o export.o collector filter collector-static filter-static

