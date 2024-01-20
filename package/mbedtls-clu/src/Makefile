
ifeq ($(CC),)
  CC=gcc
endif
ifeq ($(CP),)
  CP=cp
endif

CFLAGS:=$(CFLAGS) -Wall -Os
LIBS:=-lmbedtls -lmbedx509 -lmbedcrypto -lericstools
STATIC_OBJS:=mbedtlsclu_common.c
DEFS:=-DDEBUG -DOPENSSL_ENV_CONF_COMPAT

all: mbedtlsclu_common.o dhparam genpkey req
mbedtlsclu_common.o: mbedtlsclu_common.c
	$(CC) $(CFLAGS) $(DEFS) -c mbedtlsclu_common.c -o $@
	
dhparam: dhparam.o $(STATIC_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

dhparam.o: dhparam.c $(STATIC_OBJS)
	$(CC) $(CFLAGS) $(DEFS) -c dhparam.c -o $@

genpkey: genpkey.o $(STATIC_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

genpkey.o: genpkey.c $(STATIC_OBJS)
	$(CC) $(CFLAGS) $(DEFS) -c genpkey.c -o $@

req: req.o $(STATIC_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

req.o: req.c $(STATIC_OBJS)
	$(CC) $(CFLAGS) $(DEFS) -c req.c -o $@

clean:
	rm -rf *.o *.a *~ .*sw* mbedtlsclu_common dhparam genpkey req
