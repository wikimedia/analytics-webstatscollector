# Makefile for profile collector package
# $Id: Makefile 12318 2005-12-31 15:34:46Z midom $
#
bin_PROGRAMS = collector filter
collector_SOURCES = collector.c collector.h export.c export.h
filter_SOURCES = filter.c filter.h

#AM_LDFLAGS=-ldb -lpthread


if DEBUG
	CFLAGS = -g3 -O0 -Wall -pedantic
	CPPFLAGS = -g3 -O0 -Wall -pedantic
else
	CFLAGS = -O3 $AM_LDFLAGS
	CPPFLAGS = -O3 $AM_LDFLAGS
endif


CFLAGS = -g3 -O0 -Wall -pedantic -ldb -lpthread

#AM_CFLAGS=-ldb -lpthread
#AM_LDFLAGS=-ldb -lpthread


LIBS = $(DEPS_LIBS)
INCLUDES = $(DEPS_CFLAGS)
AM_CPPFLAGS = $(DEPS_CFLAGS)


all: collector filter

collector: collector.h collector.c export.c
#collector_LDADD = $(DEPS_LIBS) -ldb -lpthread 


filter: filter.c
	cc -o filter filter.c

export: collector.h export.c

clean:
	rm -f collector exporter

