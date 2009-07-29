
ifeq ($(CC),)
  CC=gcc
endif
ifeq ($(CP),)
  CP=cp
endif

#for ssl, uncomment the (one and only one) ssl library to link with
#USE_MATRIXSSL:=1
#USE_CYASSL:=1
USE_OPENSSL:=1

#uncomment to link to ericstools instead of compiling as static
STATIC_ERICSTOOLS:=1

CFLAGS:=$(CFLAGS) -Wall -Os
MOREFLAGS:=
LIBS:=
SSL_DEFS:=
SSL_OBJ:=
ERICSTOOLS_OBJ:=
ERICSTOOLS_HEADER:=
ERICSTOOLS_DIR:=./ericstools

ifeq ($(USE_CYASSL), 1)
	SSL_DEFS:= -DUSE_CYASSL -DHAVE_SSL
	LIBS:=$(LIBS) -lcyassl -lm 
	USE_MATRIXSSL=
	USE_OPENSSL=
endif
ifeq ($(USE_MATRIXSSL), 1)
	SSL_DEFS:= -DUSE_MATRIXSSL  -DHAVE_SSL 
	LIBS:=$(LIBS) -lmatrixssl
	SSL_OBJ=matrixssl_helper.o
	USE_CYASSL=
	USE_OPENSSL=
	
endif
ifeq ($(USE_OPENSSL), 1)
	SSL_DEFS:= -DUSE_OPENSSL -DHAVE_SSL
	LIBS:=$(LIBS) -lcrypto -lssl
	USE_MATRIXSSL=
	USE_CYASSL=
endif

ifeq ($(STATIC_ERICSTOOLS), 1)
	ERICSTOOLS_OBJ:=libericstools.a
	ERICSTOOLS_HEADER:=erics_tools.h
else
	LIBS:=$(LIBS) -lericstools
endif


MOREFLAGS:= $(SSL_DEFS) -DUSE_ERICS_TOOLS


all: ddns_updater
ddns_updater: ddns_updater.o http_minimal_client.o $(ERICSTOOLS_OBJ) $(SSL_OBJ)
	$(CC) $(LDFLAGS) $(MOREFLAGS) $^ -o $@ $(LIBS)


ddns_updater.o: ddns_updater.c $(ERICSTOOLS_HEADER)
	$(CC) $(CFLAGS) $(MOREFLAGS) -c ddns_updater.c -o $@
http_minimal_client.o: http_minimal_client.c $(ERICSTOOLS_HEADER)
	$(CC) $(CFLAGS) $(MOREFLAGS) -c http_minimal_client.c -o $@
matrixssl_helper.o: matrixssl_helper.c $(ERICSTOOLS_HEADER)
	$(CC) $(CFLAGS) $(MOREFLAGS) -c matrixssl_helper.c -o $@


erics_tools.h: libericstools.a

libericstools.a: 
	make -C $(ERICSTOOLS_DIR)
	$(CP) $(ERICSTOOLS_DIR)/libericstools.a .
	$(CP) $(ERICSTOOLS_DIR)/erics_tools.h .

clean:
	if [ -e "$(ERICSTOOLS_DIR)" ] && [ -n "$(ERICSTOOLS_DIR)" ] ; then make -C $(ERICSTOOLS_DIR) clean ; fi
	touch tmp.o tmp.a tmp~ .tmp.swp ddns_updater erics_tools.h
	rm *.o *.a *~ .*sw* ddns_updater erics_tools.h
