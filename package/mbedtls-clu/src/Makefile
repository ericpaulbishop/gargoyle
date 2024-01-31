
ifeq ($(CC),)
  CC=gcc
endif
ifeq ($(CP),)
  CP=cp
endif

CFLAGS:=$(CFLAGS) -Wall -Os
LIBS:=-lmbedtls -lmbedx509 -lmbedcrypto -lericstools
STATIC_OBJS:=mbedtlsclu_common.o
#DEFS:=-DDEBUG -DOPENSSL_ENV_CONF_COMPAT
DEFS:=-DOPENSSL_ENV_CONF_COMPAT

#all: mbedtlsclu_common.o x509write_crl.o dhparam genpkey rand req ca
all: mbedtlsclu_common.o x509write_crl.o dhparam.o genpkey.o rand.o req.o ca.o mbedtls-clu
mbedtlsclu_common.o: mbedtlsclu_common.c
	$(CC) $(CFLAGS) $(DEFS) -c mbedtlsclu_common.c -o $@

x509write_crl.o: x509write_crl.c
	$(CC) $(CFLAGS) $(DEFS) -c x509write_crl.c -o $@
	
#dhparam: dhparam.o $(STATIC_OBJS)
#	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

dhparam.o: dhparam.c $(STATIC_OBJS)
	$(CC) $(CFLAGS) $(DEFS) -c dhparam.c -o $@

#genpkey: genpkey.o $(STATIC_OBJS)
#	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

genpkey.o: genpkey.c $(STATIC_OBJS)
	$(CC) $(CFLAGS) $(DEFS) -c genpkey.c -o $@

#rand: rand.o $(STATIC_OBJS)
#	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

rand.o: rand.c $(STATIC_OBJS)
	$(CC) $(CFLAGS) $(DEFS) -c rand.c -o $@

#req: req.o $(STATIC_OBJS)
#	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

req.o: req.c $(STATIC_OBJS)
	$(CC) $(CFLAGS) $(DEFS) -c req.c -o $@

#ca: ca.o $(STATIC_OBJS)
#	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

ca.o: ca.c $(STATIC_OBJS)
	$(CC) $(CFLAGS) $(DEFS) -c ca.c -o $@

mbedtls-clu: mbedtls-clu.o $(STATIC_OBJS) ca.o dhparam.o genpkey.o rand.o req.o x509write_crl.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

mbedtls-clu.o: mbedtls-clu.c $(STATIC_OBJS)
	$(CC) $(CFLAGS) $(DEFS) -c mbedtls-clu.c -o $@

clean:
	rm -rf *.o *.a *~ .*sw* mbedtlsclu_common x509write_crl dhparam genpkey rand req ca mbedtls-clu
