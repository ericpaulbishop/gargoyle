# Makefile for httpd_gargoyle

# CONFIGURE: If you are using a SystemV-based operating system, such as
# Solaris, you will need to uncomment this definition.
#SYSV_LIBS =    -lnsl -lsocket

# CONFIGURE: Some systems don't need -lcrypt, and indeed they get an
# error if you try to link with it.  If you get an error about libcrypt
# not found, try commenting out this definition.
CRYPT_LIB =    -lcrypt

# CONFIGURE: If you want to compile in support for https, uncomment these
# definitions.  You will need to have already built OpenSSL, available at
# http://www.openssl.org/  Make sure the SSL_TREE definition points to the
# tree with your OpenSSL installation - depending on how you installed it,
# it may be in /usr/local instead of /usr/local/ssl.

# OpenSSL
#SSL_TREE =	/usr/local/ssl
#SSL_DEFS =	-DUSE_SSL
#SSL_INC =	-I${SSL_TREE}/include
#SSL_LIBS =	-L${SSL_TREE}/lib -lssl -lcrypto
#SSL_OBJS =

# MatrixSSL
SSL_TREE =
SSL_DEFS =	-DUSE_SSL -DHAVE_MATRIXSSL
SSL_INC =
SSL_LIBS =	-lmatrixssl
SSL_OBJS =	matrixssl_helper.o


DESTDIR =

BINDIR =	$(DESTDIR)/usr/sbin
MANDIR =	$(DESTDIR)/usr/share/man
CC =		gcc
CDEFS =		${SSL_DEFS} ${SSL_INC}
OFLAGS =	-O
CFLAGS =	${OFLAGS} ${CDEFS}
#CFLAGS =	-g ${CDEFS}
LDFLAGS =
#LDFLAGS =	-g
LDLIBS =	${SSL_LIBS} ${SYSV_LIBS} ${CRYPT_LIB}

all:		httpd_gargoyle

httpd_gargoyle:	httpd_gargoyle.o match.o tdate_parse.o ${SSL_OBJS}
	${CC} ${CFLAGS} ${LDFLAGS} httpd_gargoyle.o match.o tdate_parse.o ${SSL_OBJS} ${LDLIBS} -o httpd_gargoyle

httpd_gargoyle.o:	httpd_gargoyle.c version.h port.h match.h tdate_parse.h mime_encodings.h mime_types.h
	${CC} ${CFLAGS} -c httpd_gargoyle.c

matrixssl_helper.o: matrixssl_helper.c
	${CC} ${CFLAGS} -c matrixssl_helper.c
	
match.o:	match.c match.h
	${CC} ${CFLAGS} -c match.c

tdate_parse.o:	tdate_parse.c tdate_parse.h
	${CC} ${CFLAGS} -c tdate_parse.c

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
	-mkdir -p ${BINDIR}
	cp httpd_gargoyle  ${BINDIR}

uninstall:
	rm -f ${BINDIR}/httpd_gargoyle

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
