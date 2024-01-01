
ifeq ($(CC),)
  CC=gcc
endif
ifeq ($(CP),)
  CP=cp
endif


#for ssl, uncomment the (one and only one) ssl library to link with
#USE_MBEDTLS:=1
#USE_OPENSSL:=1

#uncomment to link to ericstools instead of compiling as static
#STATIC_LIBS:=1

CFLAGS:=$(CFLAGS) -Wall -Os
LIBS:=
SSL_DEFS:=
STATIC_OBJS:=
ERICSTOOLS_DIR:=./ericstools
EWGET_DIR:=./ewget

ifeq ($(USE_MBEDTLS), 1)
	SSL_DEFS:= -DUSE_MBEDTLS -DHAVE_SSL
	LIBS:=$(LIBS) -lmbedtls
	USE_MBEDTLS=1
	USE_OPENSSL=
endif
ifeq ($(USE_OPENSSL), 1)
	SSL_DEFS:= -DUSE_OPENSSL -DHAVE_SSL
	LIBS:=$(LIBS) -lcrypto -lssl
	USE_MBEDTLS=
	USE_OPENSSL=1
endif

ifeq ($(STATIC_LIBS), 1)
	STATIC_OBJS:=libericstools.a libewget.a
else
	LIBS:=$(LIBS) -lericstools -lewget
endif



OS=$(shell uname)
ifeq ($(OS),Darwin)
  CFLAGS:=$(CFLAGS) -arch i386
endif



all: ddns_updater
ddns_updater: ddns_updater.o $(STATIC_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)


ddns_updater.o: ddns_updater.c $(STATIC_OBJS)
	$(CC) $(CFLAGS) -c ddns_updater.c -o $@



libericstools.a: 
	make -C $(ERICSTOOLS_DIR)
	$(CP) $(ERICSTOOLS_DIR)/libericstools.a .
	$(CP) $(ERICSTOOLS_DIR)/erics_tools.h .

libewget.a:
	make -C $(EWGET_DIR) 
	$(CP) $(EWGET_DIR)/libewget.a .
	$(CP) $(EWGET_DIR)/ewget.h .


clean:
	if [ -e "$(ERICSTOOLS_DIR)" ] && [ -n "$(ERICSTOOLS_DIR)" ] ; then make -C $(ERICSTOOLS_DIR) clean ; fi
	if [ -e "$(EWGET_DIR)" ] && [ -n "$(EWGET_DIR)" ] ; then make -C $(EWGET_DIR) clean ; fi
	rm -rf *.o *.a *~ .*sw* ddns_updater erics_tools.h ewget.h
