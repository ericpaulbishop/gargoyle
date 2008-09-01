
#for ssl, uncomment the (one and only one) ssl library to link with
#USE_MATRIXSSL:=1
USE_OPENSSL:=1


CFLAGS:=$(CFLAGS) -Wall -Os
MOREFLAGS:=
LIBS:=
SSL_DEFS:=
SSL_OBJ:=
ifeq ($(USE_MATRIXSSL), 1)
	SSL_DEFS:= -DUSE_MATRIXSSL  -DHAVE_SSL 
	LIBS:=$(LIBS) -lmatrixssl
	SSL_OBJ=matrixssl_helper.o
endif
ifeq ($(USE_OPENSSL), 1)
	SSL_DEFS:= -DUSE_OPENSSL -DHAVE_SSL
	LIBS:=$(LIBS) -lcrypto -lssl
endif

MOREFLAGS:= $(SSL_DEFS) -DUSE_ERICS_TOOLS
LIBS:=$(LIBS) -lericstools 

all: ddns_updater
ddns_updater: ddns_updater.o http_minimal_client.o $(SSL_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(MOREFLAGS) ddns_updater.o http_minimal_client.o $(SSL_OBJ) -o $@ $(LIBS)
ddns_updater.o: ddns_updater.c
	$(CC) $(CFLAGS) $(MOREFLAGS) -c ddns_updater.c
http_minimal_client.o: http_minimal_client.c
	$(CC) $(CFLAGS) $(MOREFLAGS) -c http_minimal_client.c
matrixssl_helper.o: matrixssl_helper.c matrixssl_helper.h
	$(CC) $(CFLAGS) $(MOREFLAGS) -c matrixssl_helper.c

clean:
	touch tmp.o tmp~ .tmp.swp ddns_updater 
	rm *.o *~ .*sw* ddns_updater
