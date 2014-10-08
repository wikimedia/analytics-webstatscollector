# Makefile for profile collector package
# $Id: Makefile 12318 2005-12-31 15:34:46Z midom $
#
#MacOSX Fink library paths 
#CFLAGS+=-I/sw/include/
#LDFLAGS+=-L/sw/lib/

#MacOSX MacPorts library paths
CFLAGS+=-I/opt/local/include/
LDFLAGS+=-L/opt/local/lib/ -ldb -lpthread

#LDFLAGS+=-ldb
CFLAGS+=-Wall -g

SUBDIRS=tests

RECURSIVE_TARGETS = all-recursive clean-recursive check-recursive

all: collector filter

collector: collector.h collector.c export.c export.o
	gcc -o collector collector.c export.o -ldb -lpthread

filter: filter.c
	gcc -o filter filter.c

export.o: export.c collector.h collector.c filter.c
	gcc -c -o export.o export.c

clean:
	$(RM) *.o collector filter
	$(MAKE) clean-recursive

check: filter
	$(MAKE) check-recursive

$(RECURSIVE_TARGETS):
	list='$(SUBDIRS)' \
	target=`echo $@ | sed s/-recursive//`; \
	for subdir in $$list; do \
          echo "Making $$target in $$subdir"; \
          (cd $$subdir && $(MAKE) $$target) \
        done; \

install:
	install collector filter $(DESTDIR)/usr/local/bin
	install webstats-collector.upstart.conf $(DESTDIR)/etc/init/webstats-collector.conf
