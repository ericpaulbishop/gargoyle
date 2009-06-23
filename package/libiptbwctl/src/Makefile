VERSION=1


ifeq ($(CC),)
  CC=gcc
endif

ifeq ($(LD),)
  LD=ld
endif

ifeq ($(AR),)
  AR=ar
endif

ifeq ($(RANLIB),)
  RANLIB=ranlib
endif



OS=$(shell uname)
ifeq ($(OS),Darwin)
  LINK=$(LD)
  SHLIB_EXT=dylib
  SHLIB_FLAGS=-dylib
  SHLIB_FILE=libiptbwctl.$(SHLIB_EXT).$(VERSION)
else
  LINK=$(CC)
  SHLIB_EXT=so
  SHLIB_FILE=libiptbwctl.$(SHLIB_EXT).$(VERSION)
  SHLIB_FLAGS=-shared -Wl,-soname,$(SHLIB_FILE)
endif

CFLAGS:=$(CFLAGS) -Os
WARNING_FLAGS=-Wall -Wstrict-prototypes



all: libiptbwctl

libiptbwctl: libiptbwctl.$(SHLIB_EXT) libiptbwctl.a


libiptbwctl.a: iptbwctl_static.o
	if [ -e $@ ] ; then rm $@ ; fi
	$(AR) rc $@ $^
	$(RANLIB) $@
	

libiptbwctl.$(SHLIB_EXT) : iptbwctl_dyn.o 
	if [ -e libiptbwctl.$(SHLIB_EXT) ] ; then rm libiptbwctl.$(SHLIB_EXT)* ; fi
	$(LINK) $(LDFLAGS) $(SHLIB_FLAGS) -o $(SHLIB_FILE) $^ -lc 
	ln -s $(SHLIB_FILE) libiptbwctl.$(SHLIB_EXT)

iptbwctl_dyn.o: ipt_bwctl.c
	$(CC) $(CFLAGS) -fPIC $(WARNING_FLAGS) -o $@ -c $^

iptbwctl_static.o: ipt_bwctl.c
	$(CC) $(CFLAGS) $(WARNING_FLAGS) -o $@ -c $^




clean:
	cd utils
	rm -rf bw_get bw_set *.$(SHLIB_EXT)* *.a *.o *~ .*sw*
	cd ..
	rm -rf bw_get bw_set *.$(SHLIB_EXT)* *.a *.o *~ .*sw*
