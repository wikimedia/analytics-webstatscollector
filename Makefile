# Makefile for profile collector package
# $Id: Makefile 12318 2005-12-31 15:34:46Z midom $
#

LDLIBS+=-ldb -lpthread

CFLAGS+=-Wall -O2
# To get debug information into the executables, uncomment the
# following line
#CFLAGS+=-g

SUBDIRS=tests

RECURSIVE_TARGETS = all-recursive clean-recursive check-recursive

all: collector filter

collector.o: collector.c collector.h

collector: collector.o export.o

export.o: export.c collector.h

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
