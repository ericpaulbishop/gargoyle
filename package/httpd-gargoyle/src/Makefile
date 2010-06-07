# Makefile for httpd_gargoyle

# CONFIGURE: If you are using a SystemV-based operating system, such as
# Solaris, you will need to uncomment this definition.
#SYSV_LIBS =    -lnsl -lsocket

# CONFIGURE: Some systems don't need -lcrypt, and indeed they get an
# error if you try to link with it.  If you get an error about libcrypt
# not found, try commenting out this definition.
CRYPT_LIB =    -lcrypt


#for ssl, uncomment the (one and only one) ssl library to link with
#USE_MATRIXSSL:=1
#USE_CYASSL:=1
USE_OPENSSL:=1

ifeq ($(USE_CYASSL), 1)
	SSL_DEFS:=-DUSE_CYASSL -DHAVE_SSL
	SSL_LIBS:=-lcyassl -lm
       	ifeq ($(USE_CYASSL_INCLUDE_DIR), 1)
		SSL_DEFS:=$(SSL_DEFS) -DUSE_CYASSL_INCLUDE_DIR
	endif
	SSL_OBJ=
	USE_MATRIXSSL=
	USE_OPENSSL=
endif
ifeq ($(USE_MATRIXSSL), 1)
	SSL_DEFS:=-DUSE_MATRIXSSL  -DHAVE_SSL 
	SSL_LIBS:=-lmatrixssl
	SSL_OBJ=matrixssl_helper.o
	USE_CYASSL=
	USE_OPENSSL=
	
endif
ifeq ($(USE_OPENSSL), 1)
	SSL_DEFS:=-DUSE_OPENSSL -DHAVE_SSL
	SSL_LIBS:=-lcrypto -lssl
	SSL_OBJ=
	USE_MATRIXSSL=
	USE_CYASSL=
endif





DESTDIR =

BINDIR =	$(DESTDIR)/usr/sbin
MANDIR =	$(DESTDIR)/usr/share/man
CC =		gcc
OFLAGS =	-O
CFLAGS =	$(OFLAGS)
LDFLAGS =
LDLIBS =	$(SSL_LIBS) $(SYSV_LIBS) $(CRYPT_LIB)

all:		httpd_gargoyle

httpd_gargoyle:	httpd_gargoyle.o match.o tdate_parse.o $(SSL_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SSL_DEFS) httpd_gargoyle.o match.o tdate_parse.o $(SSL_OBJ) $(LDLIBS) -o httpd_gargoyle



httpd_gargoyle.o:	httpd_gargoyle.c version.h port.h match.h tdate_parse.h mime_encodings.h mime_types.h
	$(CC) $(CFLAGS) $(SSL_DEFS)  -c httpd_gargoyle.c


matrixssl_helper.o: matrixssl_helper.c
	$(CC) $(CFLAGS) $(SSL_DEFS) -c matrixssl_helper.c
	
match.o:	match.c match.h
	$(CC) $(CFLAGS) $(SSL_DEFS) -c match.c

tdate_parse.o:	tdate_parse.c tdate_parse.h
	$(CC) $(CFLAGS) $(SSL_DEFS) -c tdate_parse.c

mime_encodings.h: mime_encodings.txt
	rm -f mime_encodings.h
	sed < mime_encodings.txt > mime_encodings.h \
	  -e 's/#.*//' -e 's/[ 	]*$$//' -e '/^$$/d' \
	  -e 's/[ 	][ 	]*/", 0, "/' -e 's/^/{ "/' -e 's/$$/", 0 },/'

mime_types.h: mime_types.txt
	rm -f mime_types.h
	sed < mime_types.txt > mime_types.h \
	  -e 's/#.*//' -e 's/[ 	]*$$//' -e '/^$$/d' \
	  -e 's/[ 	][ 	]*/", 0, "/' -e 's/^/{ "/' -e 's/$$/", 0 },/'




cert:		httpd_gargoyle.pem
httpd_gargoyle.pem:	httpd_gargoyle.cnf
	openssl req -new -x509 -days 7500 -nodes -config httpd_gargoyle.cnf -out httpd_gargoyle.pem -keyout httpd_gargoyle.pem
	openssl x509 -subject -dates -fingerprint -noout -in httpd_gargoyle.pem
	chmod 600 httpd_gargoyle.pem


install:	all uninstall
	-mkdir -p $(BINDIR)
	cp httpd_gargoyle  $(BINDIR)

uninstall:
	rm -f $(BINDIR)/httpd_gargoyle

clean:
	rm -f httpd_gargoyle mime_encodings.h mime_types.h httpd_gargoyle.rnd *.o core core.* *.core

tar:
	@name=`sed -n -e '/SERVER_SOFTWARE/!d' -e 's,.*httpd_gargoyle/,httpd_gargoyle-,' -e 's, .*,,p' version.h` ; \
	  rm -rf $$name ; \
	  mkdir $$name ; \
	  tar cf - `cat FILES` | ( cd $$name ; tar xfBp - ) ; \
	  chmod 644 $$name/Makefile $$name/mime_encodings.txt $$name/mime_types.txt ; \
	  chmod 755 $$name/contrib $$name/contrib/redhat-rpm ; \
	  tar cf $$name.tar $$name ; \
	  rm -rf $$name ; \
	  gzip $$name.tar
