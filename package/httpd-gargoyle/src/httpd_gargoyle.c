/*  httpd_gargoyle -	A small HTTP server for use in Gargoyle, 
 * 			An open-source web interface for OpenWrt
 * 			Created By Eric Bishop
 * 			http://www.gargoyle-router.com
 * 		  
 * 			This program is a modified version of mini_httpd
 * 		  	The original mini_httpd was created by Jef Poscanzer
 * 		  	http://www.acme.com/software/mini_httpd/
 *
 *  Copyright © 2008-2011 by Eric Bishop <eric@gargoyle-router.com>
 * 
 *  This file is free software: you may copy, redistribute and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This file is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *	Copyright © 1999,2000 by Jef Poskanzer <jef@acme.com>.
 *	All rights reserved.
 *
 *
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions
 *	are met:
 *	1. Redistributions of source code must retain the above copyright
 *	   notice, this list of conditions and the following disclaimer.
 *	2. Redistributions in binary form must reproduce the above copyright
 *	   notice, this list of conditions and the following disclaimer in the
 *	   documentation and/or other materials provided with the distribution.
 *
 *	THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *	ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 *	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *	OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *	OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *	SUCH DAMAGE.
*/


#include "version.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <pwd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>

#include "port.h"
#include "match.h"
#include "tdate_parse.h"

#ifdef HAVE_SENDFILE
# ifdef HAVE_LINUX_SENDFILE
#  include <sys/sendfile.h>
# else /* HAVE_LINUX_SENDFILE */
#  include <sys/uio.h>
# endif /* HAVE_LINUX_SENDFILE */
#endif /* HAVE_SENDFILE */

#ifdef HAVE_SSL
	#ifdef USE_OPENSSL
		#include <openssl/ssl.h>
		#include <openssl/err.h>
	#endif
	#ifdef USE_CYASSL
		#ifdef USE_CYASSL_INCLUDE_DIR
			#include <cyassl/openssl/ssl.h>
			#include <cyassl/openssl/err.h>
			#include <cyassl/cyassl_error.h>
		#else
			#include <openssl/ssl.h>
			#include <openssl/err.h>
			#include <cyassl_error.h>
		#endif
	#endif
	#ifdef USE_MATRIXSSL
		#include "matrixssl_helper.h"
	#endif 
#endif

extern char* crypt( const char* key, const char* setting );


#if defined(AF_INET6) && defined(IN6_IS_ADDR_V4MAPPED)
#define USE_IPV6
#endif

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#ifndef SHUT_WR
#define SHUT_WR 1
#endif

#ifndef SIZE_T_MAX
#define SIZE_T_MAX 2147483647L
#endif

#ifndef HAVE_INT64T
typedef long long int64_t;
#endif

#ifdef __CYGWIN__
#define timezone  _timezone
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif


#ifndef ERR_DIR
#define ERR_DIR "errors"
#endif /* ERR_DIR */
#ifndef DEFAULT_HTTP_PORT
#define DEFAULT_HTTP_PORT 80
#endif /* DEFAULT_HTTP_PORT */
#ifdef HAVE_SSL
#ifndef DEFAULT_HTTPS_PORT
#define DEFAULT_HTTPS_PORT 443
#endif /* DEFAULT_HTTPS_PORT */
#ifndef DEFAULT_CERTFILE
#define DEFAULT_CERTFILE "httpd_gargoyle.pem"
#endif /* DEFAULT_CERTFILE */
#endif /* HAVE_SSL */
#ifndef DEFAULT_USER
#define DEFAULT_USER "nobody"
#endif /* DEFAULT_USER */
#ifndef CGI_NICE
#define CGI_NICE 10
#endif /* CGI_NICE */
#ifndef CGI_PATH
#define CGI_PATH "/usr/local/bin:/usr/ucb:/bin:/usr/bin:/sbin:/usr/sbin"
#endif /* CGI_PATH */
#ifndef CGI_LD_LIBRARY_PATH
#define CGI_LD_LIBRARY_PATH "/usr/local/lib:/usr/lib:/lib"
#endif /* CGI_LD_LIBRARY_PATH */
#ifndef AUTH_FILE
#define AUTH_FILE ".htpasswd"
#endif /* AUTH_FILE */
#ifndef READ_TIMEOUT
#define READ_TIMEOUT 60
#endif /* READ_TIMEOUT */
#ifndef WRITE_TIMEOUT
#define WRITE_TIMEOUT 300
#endif /* WRITE_TIMEOUT */
#ifndef DEFAULT_CHARSET
#define DEFAULT_CHARSET "utf-8"
#endif /* DEFAULT_CHARSET */


#define METHOD_UNKNOWN 0
#define METHOD_GET 1
#define METHOD_HEAD 2
#define METHOD_POST 3


/* A multi-family sockaddr. */
typedef union {
    struct sockaddr sa;
    struct sockaddr_in sa_in;
#ifdef USE_IPV6
    struct sockaddr_in6 sa_in6;
    struct sockaddr_storage sa_stor;
#endif /* USE_IPV6 */
    } usockaddr;


static char* argv0;
static int debug;
static unsigned short port;
static char* dir;
static char* data_dir;
static int do_chroot;
static int vhost;
static char* user;
static char* cgi_pattern;
static char* url_pattern;
static int no_empty_referers;
static char* local_pattern;
static char* hostname;
static char hostname_buf[500];
static char* logfile;
static char* pidfile;
static char* charset;
static char* p3p;
static int max_age;
static FILE* logfp;
static int listen4_fd, listen6_fd;
#ifdef HAVE_SSL
	static int do_ssl;
	static char* certfile;
	static char* cipher;
	#ifdef USE_OPENSSL
		static SSL_CTX* ssl_ctx;
	#endif
	#ifdef USE_CYASSL
		static SSL_CTX* ssl_ctx;
	#endif
	#ifdef USE_MATRIXSSL
		static sslKeys_t* keys;
	#endif
#endif
static char cwd[MAXPATHLEN];
static int got_hup;

/*added for gargoyle version*/
static char* defaultRealmName;
static char* defaultRealmPasswordFile;
static char* defaultPageFile;
static char* pageNotFoundFile;
static unsigned char allowDirectoryListing;
static unsigned short sslPort; 
static int listen4s_fd, listen6s_fd;
/* end gargoyle variables */

/* Request variables. */
static int conn_fd;
#ifdef HAVE_SSL
static SSL* ssl;
#endif /* HAVE_SSL */
static usockaddr client_addr;
static char* request;
static size_t request_size, request_len, request_idx;
static int method;
static char* path;
static char* file;
static char* pathinfo;
struct stat sb;
static char* query;
static char* protocol;
static int status;
static off_t bytes;
static char* req_hostname;

static char* authorization;
static size_t content_length;
static char* content_type;
static char* cookie;
static char* host;
static time_t if_modified_since;
static char* referer;
static char* useragent;

static char* remoteuser;


/* Forwards. */
static void usage( void );
static void read_config( char* filename );
static void value_required( char* name, char* value );
static void no_value_required( char* name, char* value );
static int initialize_listen_socket( usockaddr* usaP );
static void handle_request( int is_ssl, unsigned short conn_port );
static void de_dotdot( char* file );
static int get_pathinfo( void );
static void do_file( int is_ssl, unsigned short conn_port );
static void do_dir( int is_ssl );
#ifdef HAVE_SCANDIR
static char* file_details( const char* dir, const char* name );
static void strencode( char* to, size_t tosize, const char* from );
#endif /* HAVE_SCANDIR */
static void do_cgi( int is_ssl, unsigned short port );
static void cgi_interpose_input( int wfd, int is_ssl );
static void post_post_garbage_hack( int is_ssl );
static void cgi_interpose_output( int rfd, int parse_headers, int is_ssl );
static char** make_argp( void );
static char** make_envp( int is_sl, unsigned short port );
static char* build_env( char* fmt, char* arg );
static void auth_check( char* dirname, int is_ssl);
static void send_authenticate( char* realm, int is_ssl );
static char* virtual_file( char* file );
static void send_error( int s, char* title, char* extra_header, char* text, int is_ssl );
static void send_redirect( char* extra_header, char* hostname, char* new_location, int is_ssl);
static void send_error_body( int s, char* title, char* text );
static int send_error_file( char* filename );
static void send_error_tail( void );
static void add_headers( int s, char* title, char* extra_header, char* me, char* mt, off_t b, time_t mod );
static void start_request( void );
static void add_to_request( char* str, size_t len );
static char* get_request_line( void );
static void start_response( void );
static void add_to_response( char* str, size_t len );
static void send_response( int is_ssl );
static void send_via_write( int fd, off_t size, int is_ssl );
static ssize_t my_read( char* buf, size_t size, int is_ssl );
static ssize_t my_write( char* buf, size_t size, int is_ssl );
#ifdef HAVE_SENDFILE
static int my_sendfile( int fd, int socket, off_t offset, size_t nbytes );
#endif /* HAVE_SENDFILE */
static void add_to_buf( char** bufP, size_t* bufsizeP, size_t* buflenP, char* str, size_t len );
static void make_log_entry( void );
static void check_referer( int is_ssl );
static int really_check_referer( void );
static char* get_method_str( int m );
static void init_mime( void );
static const char* figure_mime( char* name, char* me, size_t me_size );
static void handle_sigterm( int sig );
static void handle_sighup( int sig );
static void handle_sigchld( int sig );
static void re_open_logfile( void );
static void handle_read_timeout( int sig, int is_ssl );
static void handle_read_timeout_sig(int sig);
static void handle_write_timeout( int sig );

static void lookup_hostname(	usockaddr* usa4P, 
				usockaddr* usa4sP, 
				size_t sa4_len, 
				int* gotv4P,
			        int* gotv4sP,	
				usockaddr* usa6P, 
				usockaddr* usa6sP, 
				size_t sa6_len, 
				int* gotv6P,
				int* gotv6sP
				);

static char* ntoa( usockaddr* usaP );
static int sockaddr_check( usockaddr* usaP );
static size_t sockaddr_len( usockaddr* usaP );
static void strdecode( char* to, char* from );
static int hexit( char c );
static int b64_decode( const char* str, unsigned char* space, int size );
static void set_ndelay( int fd );
static void clear_ndelay( int fd );
static void* e_malloc( size_t size );
static void* e_realloc( void* optr, size_t size );
static char* e_strdup( char* ostr );
#ifdef NO_SNPRINTF
static int snprintf( char* str, size_t size, const char* format, ... );
#endif /* NO_SNPRINTF */

static char* default_content_type;

int
main( int argc, char** argv )
    {

    int argn;
    struct passwd* pwd;
    uid_t uid = 32767;
    gid_t gid = 32767;
    usockaddr host_addr4;
    usockaddr host_addr6;
    int gotv4, gotv4s, gotv6, gotv6s;
    fd_set lfdset;
    int maxfd;
    usockaddr usa;
    int sz, r;
    char* cp;

    /* gargoyle main vars */
    usockaddr host_addr4s;
    usockaddr host_addr6s;
    /* end gargoyle main vars */


    /* Parse args. */
    argv0 = argv[0];
    debug = 0;
    port = 0;
    dir = (char*) 0;
    data_dir = (char*) 0;
    do_chroot = 0;
    vhost = 0;
    cgi_pattern = (char*) 0;
    url_pattern = (char*) 0;
    no_empty_referers = 0;
    local_pattern = (char*) 0;
    charset = DEFAULT_CHARSET;
    p3p = (char*) 0;
    max_age = -1;
    user = DEFAULT_USER;
    hostname = (char*) 0;
    logfile = (char*) 0;
    pidfile = (char*) 0;
    logfp = (FILE*) 0;
#ifdef HAVE_SSL
    do_ssl = 0;
    certfile = DEFAULT_CERTFILE;
    cipher = (char*) 0;
#endif /* HAVE_SSL */

     /* added gargoyle variable defaults */
    defaultRealmName = NULL;
    defaultRealmPasswordFile = NULL;
    defaultPageFile = NULL;
    pageNotFoundFile = NULL;
    allowDirectoryListing = 1;
    sslPort = 0;
    /* end added gargoyle defaults */


    argn = 1;
    while ( argn < argc && argv[argn][0] == '-' )
	{
	if ( strcmp( argv[argn], "-V" ) == 0 )
	    {
	    (void) printf( "%s\n", SERVER_SOFTWARE );
	    exit( 0 );
	    }
	else if ( strcmp( argv[argn], "-C" ) == 0 && argn + 1 < argc )
	    {
	    ++argn;
	    read_config( argv[argn] );
	    }
	else if ( strcmp( argv[argn], "-D" ) == 0 )
	    debug = 1;
#ifdef HAVE_SSL
	else if ( strcmp( argv[argn], "-S" ) == 0 )
	    do_ssl = 1;
	else if ( strcmp( argv[argn], "-E" ) == 0 && argn + 1 < argc )
	    {
	    ++argn;
	    certfile = argv[argn];
	    }
	else if ( strcmp( argv[argn], "-Y" ) == 0 && argn + 1 < argc )
	    {
	    ++argn;
	    cipher = argv[argn];
	    }
#endif /* HAVE_SSL */
	else if ( strcmp( argv[argn], "-p" ) == 0 && argn + 1 < argc )
	    {
	    ++argn;
	    port = (unsigned short) atoi( argv[argn] );
	    }
	else if ( strcmp( argv[argn], "-d" ) == 0 && argn + 1 < argc )
	    {
	    ++argn;
	    dir = argv[argn];
	    }
	else if ( strcmp( argv[argn], "-dd" ) == 0 && argn + 1 < argc )
	    {
	    ++argn;
	    data_dir = argv[argn];
	    }
	else if ( strcmp( argv[argn], "-c" ) == 0 && argn + 1 < argc )
	    {
	    ++argn;
	    cgi_pattern = argv[argn];
	    }
	else if ( strcmp( argv[argn], "-u" ) == 0 && argn + 1 < argc )
	    {
	    ++argn;
	    user = argv[argn];
	    }
	else if ( strcmp( argv[argn], "-h" ) == 0 && argn + 1 < argc )
	    {
	    ++argn;
	    hostname = argv[argn];
	    }
	else if ( strcmp( argv[argn], "-r" ) == 0 )
	    do_chroot = 1;
	else if ( strcmp( argv[argn], "-v" ) == 0 )
	    vhost = 1;
	else if ( strcmp( argv[argn], "-l" ) == 0 && argn + 1 < argc )
	    {
	    ++argn;
	    logfile = argv[argn];
	    }
	else if ( strcmp( argv[argn], "-i" ) == 0 && argn + 1 < argc )
	    {
	    ++argn;
	    pidfile = argv[argn];
	    }
	else if ( strcmp( argv[argn], "-T" ) == 0 && argn + 1 < argc )
	    {
	    ++argn;
	    charset = argv[argn];
	    }
	else if ( strcmp( argv[argn], "-P" ) == 0 && argn + 1 < argc )
	    {
	    ++argn;
	    p3p = argv[argn];
	    }
	else if ( strcmp( argv[argn], "-M" ) == 0 && argn + 1 < argc )
	    {
	    ++argn;
	    max_age = atoi( argv[argn] );
	    }
	/* parse gargoyle variables */
	else if ( strcmp( argv[argn], "-DRN" ) == 0 && argn + 1 < argc )
	{
		++argn;
		defaultRealmName = argv[argn];
	}
	else if ( strcmp( argv[argn], "-DRP" ) == 0 && argn + 1 < argc )
	{
		++argn;
		defaultRealmPasswordFile = argv[argn];
	}
	else if( strcmp( argv[argn], "-DPF" ) == 0 && argn + 1 < argc )
	{
		++argn;
		defaultPageFile = argv[argn];
	}
	else if( strcmp( argv[argn], "-PNF" ) == 0 && argn + 1 < argc )
	{
		++argn;
		pageNotFoundFile = argv[argn];
	}
	else if( strcmp( argv[argn], "-ADL") == 0 && argn + 1 < argc )
	{
		++argn;
		if(strcmp(argv[argn], "1") == 0 || strcmp(argv[argn], "true") == 0 || strcmp(argv[argn], "TRUE") == 0 || strcmp(argv[argn], "yes") == 0 || strcmp(argv[argn], "YES") == 0 )
		{
			allowDirectoryListing = 1;
		}
		else
		{
			allowDirectoryListing = 0;
		}
	}	

#ifdef HAVE_SSL
	else if( strcmp( argv[argn], "-SP" ) == 0 && argn + 1 < argc )
	{
		++argn;
		sslPort = (unsigned short) atoi( argv[argn] );
	}
#endif
	/* end gargoyle variables */
	else
	    usage();
	++argn;
	}
    if ( argn != argc )
	usage();


    /* set default_content_type */
    default_content_type = (char*)e_malloc(strlen("text/html; charset=") +  strlen(charset) + 1);
    sprintf(default_content_type, "text/html; charset=%s", charset);



    cp = strrchr( argv0, '/' );
    if ( cp != (char*) 0 )
	++cp;
    else
	cp = argv0;
    openlog( cp, LOG_NDELAY|LOG_PID, LOG_DAEMON );


    if ( port == 0 )
	{
#ifdef HAVE_SSL
	/* gargoyle addition */
	if(sslPort != 0 && do_ssl)
	{
		port = sslPort;
		sslPort = 0;
	}
	/* end gargoyle addition */
	if ( do_ssl )
	    port = DEFAULT_HTTPS_PORT;
	else
	    port = DEFAULT_HTTP_PORT;
#else /* HAVE_SSL */
	port = DEFAULT_HTTP_PORT;
#endif /* HAVE_SSL */
	}

    /* If we're root and we're going to become another user, get the uid/gid
    ** now.
    */
    if ( getuid() == 0 )
	{
	pwd = getpwnam( user );
	if ( pwd == (struct passwd*) 0 )
	    {
	    syslog( LOG_CRIT, "unknown user - '%s'", user );
	    (void) fprintf( stderr, "%s: unknown user - '%s'\n", argv0, user );
	    exit( 1 );
	    }
	uid = pwd->pw_uid;
	gid = pwd->pw_gid;
	}

    /* Log file. */
    if ( logfile != (char*) 0 )
	{
	logfp = fopen( logfile, "a" );
	if ( logfp == (FILE*) 0 )
	    {
	    syslog( LOG_CRIT, "%s - %m", logfile );
	    perror( logfile );
	    exit( 1 );
	    }
	if ( logfile[0] != '/' )
	    {
	    syslog( LOG_WARNING, "logfile is not an absolute path, you may not be able to re-open it" );
	    (void) fprintf( stderr, "%s: logfile is not an absolute path, you may not be able to re-open it\n", argv0 );
	    }
	if ( getuid() == 0 )
	    {
	    /* If we are root then we chown the log file to the user we'll
	    ** be switching to.
	    */
	    if ( fchown( fileno( logfp ), uid, gid ) < 0 )
		{
		syslog( LOG_WARNING, "fchown logfile - %m" );
		perror( "fchown logfile" );
		}
	    }
	}

    /* Look up hostname. */
    lookup_hostname(
	&host_addr4, &host_addr4s, sizeof(struct sockaddr_in),  &gotv4, &gotv4s,
	&host_addr6, &host_addr6s, sizeof(struct sockaddr_in6), &gotv6, &gotv6s );

    if ( hostname == (char*) 0 )
	{
	(void) gethostname( hostname_buf, sizeof(hostname_buf) );
	hostname = hostname_buf;
	}
    if ( ! ( gotv4 || gotv4s || gotv6 || gotv6s ) )
	{
	syslog( LOG_CRIT, "can't find any valid address" );
	(void) fprintf( stderr, "%s: can't find any valid address\n", argv0 );
	exit( 1 );
	}

    /* Initialize listen sockets.  Try v6 first because of a Linux peculiarity;
    ** like some other systems, it has magical v6 sockets that also listen for
    ** v4, but in Linux if you bind a v4 socket first then the v6 bind fails.
    */



    listen6_fd  = -1;
    listen6s_fd = -1;
    listen4_fd  = -1;
    listen4s_fd = -1;
    if ( gotv6 )
    {
	   //syslog(LOG_CRIT, "initializing 6\n" );
	   listen6_fd = initialize_listen_socket( &host_addr6 );
    }
    if( gotv6s )
    {
	   //syslog(LOG_CRIT, "initializing 6s\n" );
	   listen6s_fd = initialize_listen_socket( &host_addr6s );
    }  
    if ( gotv4 )
    {
	   //syslog(LOG_CRIT, "initializing 4\n" );
	   listen4_fd = initialize_listen_socket( &host_addr4 );
    }
    if( gotv4s )
    {
	   //syslog(LOG_CRIT, "initializing 4s\n" );
	   listen4s_fd = initialize_listen_socket( &host_addr4s );
    }


    /* If we didn't get any valid sockets, fail. */
    if ( listen4_fd == -1 && listen6_fd == -1 && listen4s_fd == -1 && listen6s_fd == -1 )
	{
	syslog( LOG_CRIT, "can't bind to any address" );
	(void) fprintf( stderr, "%s: can't bind to any address\n", argv0 );
	exit( 1 );
	}

#ifdef HAVE_SSL
	if ( do_ssl )
	{
	#ifdef USE_OPENSSL
		SSL_load_error_strings();
		SSLeay_add_ssl_algorithms();
		ssl_ctx = SSL_CTX_new( SSLv23_server_method() );
		if ( certfile[0] != '\0' )
		{
			if (	SSL_CTX_use_certificate_file( ssl_ctx, certfile, SSL_FILETYPE_PEM ) == 0 ||
				SSL_CTX_use_PrivateKey_file( ssl_ctx, certfile, SSL_FILETYPE_PEM ) == 0 ||
				SSL_CTX_check_private_key( ssl_ctx ) == 0 
				)
			{
				ERR_print_errors_fp( stderr );
				exit( 1 );
			}
		}
		if ( cipher != (char*) 0 )
		{
			if ( SSL_CTX_set_cipher_list( ssl_ctx, cipher ) == 0 )
			{
				ERR_print_errors_fp( stderr );
				exit( 1 );
			}
		}
	#endif
	#ifdef USE_CYASSL
		ssl_ctx = SSL_CTX_new( TLSv1_server_method() );
		SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, 0);
		if ( certfile[0] != '\0' )
		{
			if (	SSL_CTX_use_certificate_file( ssl_ctx, certfile, SSL_FILETYPE_PEM ) != SSL_SUCCESS ||
				SSL_CTX_use_PrivateKey_file( ssl_ctx, certfile, SSL_FILETYPE_PEM ) != SSL_SUCCESS 
				//|| SSL_CTX_check_private_key( ssl_ctx ) != SSL_SUCCESS
				)
			{
				syslog( LOG_CRIT, "can't load certificate and/or private key\n");
				exit( 1 );
			}
		}
		if ( cipher != (char*) 0 )
		{
			if ( SSL_CTX_set_cipher_list( ssl_ctx, cipher ) <= 0 )
			{
				syslog( LOG_CRIT, "can't load certificate and/or private key\n");
				exit( 1 );
			}
		}
	#endif
	#ifdef USE_MATRIXSSL
		matrixSslOpen();
		if ( matrixSslReadKeys( &keys, certfile, certfile, NULL, NULL ) < 0 )
		{
			syslog( LOG_CRIT, "can't load certificate and/or private key\n");
			(void) fprintf( stderr, "%s: can't load certificate and/or private key\n", argv0 );
			exit( 1 );
		}
	#endif
	}
#endif /* HAVE_SSL */




    if ( ! debug )
	{
	/* Make ourselves a daemon. */
#ifdef HAVE_DAEMON
	if ( daemon( 1, 0 ) < 0 )
	    {
	    syslog( LOG_CRIT, "daemon - %m" );
	    perror( "daemon" );
	    exit( 1 );
	    }
#else
	switch ( fork() )
	    {
	    case 0:
	    fclose(stdin);
	    fclose(stdout);
	    fclose(stderr);
	    break;
	    case -1:
	    syslog( LOG_CRIT, "fork - %m" );
	    perror( "fork" );
	    exit( 1 );
	    default:
	    exit( 0 );
	    }
#ifdef HAVE_SETSID
	(void) setsid();
#endif
#endif
	}
    else
	{
	/* Even if we don't daemonize, we still want to disown our parent
	** process.
	*/
#ifdef HAVE_SETSID
	(void) setsid();
#endif /* HAVE_SETSID */
	}

    if ( pidfile != (char*) 0 )
        {
	/* Write the PID file. */
	FILE* pidfp = fopen( pidfile, "w" );
        if ( pidfp == (FILE*) 0 )
            {
	    syslog( LOG_CRIT, "%s - %m", pidfile );
	    perror( pidfile );
            exit( 1 );
            }
        (void) fprintf( pidfp, "%d\n", (int) getpid() );
        (void) fclose( pidfp );
        }

    /* Read zone info now, in case we chroot(). */
    tzset();

    /* If we're root, start becoming someone else. */
    if ( getuid() == 0 )
	{
	/* Set aux groups to null. */
	if ( setgroups( 0, (gid_t*) 0 ) < 0 )
	    {
	    syslog( LOG_CRIT, "setgroups - %m" );
	    perror( "setgroups" );
	    exit( 1 );
	    }
	/* Set primary group. */
	if ( setgid( gid ) < 0 )
	    {
	    syslog( LOG_CRIT, "setgid - %m" );
	    perror( "setgid" );
	    exit( 1 );
	    }
	/* Try setting aux groups correctly - not critical if this fails. */
	if ( initgroups( user, gid ) < 0 )
	    {
	    syslog( LOG_ERR, "initgroups - %m" );
	    perror( "initgroups" );
	    }
#ifdef HAVE_SETLOGIN
	/* Set login name. */
	(void) setlogin( user );
#endif /* HAVE_SETLOGIN */
	}

    /* Switch directories if requested. */
    if ( dir != (char*) 0 )
	{
	if ( chdir( dir ) < 0 )
	    {
	    syslog( LOG_CRIT, "chdir - %m" );
	    perror( "chdir" );
	    exit( 1 );
	    }
	}

    /* Get current directory. */
    if( getcwd( cwd, sizeof(cwd) - 1 ) == NULL) { ; }
    if ( cwd[strlen( cwd ) - 1] != '/' )
	(void) strcat( cwd, "/" );

    /* Chroot if requested. */
    if ( do_chroot )
	{
	if ( chroot( cwd ) < 0 )
	    {
	    syslog( LOG_CRIT, "chroot - %m" );
	    perror( "chroot" );
	    exit( 1 );
	    }
	/* If we're logging and the logfile's pathname begins with the
	** chroot tree's pathname, then elide the chroot pathname so
	** that the logfile pathname still works from inside the chroot
	** tree.
	*/
	if ( logfile != (char*) 0 )
	    if ( strncmp( logfile, cwd, strlen( cwd ) ) == 0 )
		{
		(void) strcpy( logfile, &logfile[strlen( cwd ) - 1] );
		/* (We already guaranteed that cwd ends with a slash, so leaving
		** that slash in logfile makes it an absolute pathname within
		** the chroot tree.)
		*/
		}
	    else
		{
		syslog( LOG_WARNING, "logfile is not within the chroot tree, you will not be able to re-open it" );
		(void) fprintf( stderr, "%s: logfile is not within the chroot tree, you will not be able to re-open it\n", argv0 );
		}
	(void) strcpy( cwd, "/" );
	/* Always chdir to / after a chroot. */
	if ( chdir( cwd ) < 0 )
	    {
	    syslog( LOG_CRIT, "chroot chdir - %m" );
	    perror( "chroot chdir" );
	    exit( 1 );
	    }

	}

    /* Switch directories again if requested. */
    if ( data_dir != (char*) 0 )
	{
	if ( chdir( data_dir ) < 0 )
	    {
	    syslog( LOG_CRIT, "data_dir chdir - %m" );
	    perror( "data_dir chdir" );
	    exit( 1 );
	    }
	}

    /* If we're root, become someone else. */
    if ( getuid() == 0 )
	{
	/* Set uid. */
	if ( setuid( uid ) < 0 )
	    {
	    syslog( LOG_CRIT, "setuid - %m" );
	    perror( "setuid" );
	    exit( 1 );
	    }
	/* Check for unnecessary security exposure. */
	if ( ! do_chroot )
	    {
	    syslog( LOG_WARNING,
		"started as root without requesting chroot(), warning only" );
	    (void) fprintf( stderr,
		"%s: started as root without requesting chroot(), warning only\n", argv0 );
	    }
	}

    /* Catch various signals. */
#ifdef HAVE_SIGSET
    (void) sigset( SIGTERM, handle_sigterm );
    (void) sigset( SIGINT, handle_sigterm );
    (void) sigset( SIGUSR1, handle_sigterm );
    (void) sigset( SIGHUP, handle_sighup );
    (void) sigset( SIGCHLD, handle_sigchld );
    (void) sigset( SIGPIPE, SIG_IGN );
#else /* HAVE_SIGSET */
    (void) signal( SIGTERM, handle_sigterm );
    (void) signal( SIGINT, handle_sigterm );
    (void) signal( SIGUSR1, handle_sigterm );
    (void) signal( SIGHUP, handle_sighup );
    (void) signal( SIGCHLD, handle_sigchld );
    (void) signal( SIGPIPE, SIG_IGN );
#endif /* HAVE_SIGSET */
    got_hup = 0;

    init_mime();

    if ( hostname == (char*) 0 )
	syslog(
	    LOG_NOTICE, "%.80s starting on port %d", SERVER_SOFTWARE,
	    (int) port );
    else
	syslog(
	    LOG_NOTICE, "%.80s starting on %.80s, port %d", SERVER_SOFTWARE,
	    hostname, (int) port );

    /* Main loop. */
    for (;;)
	{
	int is_ssl;
	unsigned short conn_port;

	/* Do we need to re-open the log file? */
	if ( got_hup )
	    {
	    re_open_logfile();
	    got_hup = 0;
	    }

	/* Do a select() on at least one and possibly two listen fds.
	** If there's only one listen fd then we could skip the select
	** and just do the (blocking) accept(), saving one system call;
	** that's what happened up through version 1.18.  However there
	** is one slight drawback to that method: the blocking accept()
	** is not interrupted by a signal call.  Since we definitely want
	** signals to interrupt a waiting server, we use select() even
	** if there's only one fd.
	*/
	FD_ZERO( &lfdset );
	maxfd = -1;
	if ( listen4_fd != -1 )
	    {
	    FD_SET( listen4_fd, &lfdset );
	    if ( listen4_fd > maxfd )
		maxfd = listen4_fd;
	    }
	if ( listen4s_fd != -1 )
	    {
	    FD_SET( listen4s_fd, &lfdset );
	    if ( listen4s_fd > maxfd )
		maxfd = listen4s_fd;
	    }

	if ( listen6_fd != -1 )
	    {
	    FD_SET( listen6_fd, &lfdset );
	    if ( listen6_fd > maxfd )
		maxfd = listen6_fd;
	    }
	if ( listen6s_fd != -1 )
	    {
	    FD_SET( listen6s_fd, &lfdset );
	    if ( listen6s_fd > maxfd )
		maxfd = listen6s_fd;
	    }


	if ( select( maxfd + 1, &lfdset, (fd_set*) 0, (fd_set*) 0, (struct timeval*) 0 ) < 0 )
	    {
	    if ( errno == EINTR || errno == EAGAIN )
		continue;	/* try again */
	    syslog( LOG_CRIT, "select - %m" );
	    perror( "select" );
	    exit( 1 );
	    }


	/* determine whether this is ssl connection */
#ifdef HAVE_SSL
	is_ssl= 0;
	conn_port = port;
	if( do_ssl)
	{
		if(sslPort == 0)
		{
			is_ssl= 1;
		}
		else if( listen4s_fd != -1 && FD_ISSET( listen4s_fd, &lfdset ))
		{
			is_ssl = 1;
			conn_port = sslPort;
		}
		else if( listen6s_fd != -1 && FD_ISSET( listen6s_fd, &lfdset ) )
		{
			is_ssl = 1;
			conn_port = sslPort;
		}
	}
#else /* HAVE_SSL */
	is_ssl = 0;
	conn_port = port;
#endif /* HAVE_SSL */

	/* Accept the new connection. */
	sz = sizeof(usa);
	if ( listen4_fd != -1 && FD_ISSET( listen4_fd, &lfdset ) ) { conn_fd = accept( listen4_fd, &usa.sa, &sz ); }
	else if ( listen4s_fd != -1 && FD_ISSET( listen4s_fd, &lfdset ) ) { conn_fd = accept( listen4s_fd, &usa.sa, &sz ); }
	else if ( listen6_fd != -1 && FD_ISSET( listen6_fd, &lfdset ) ) { conn_fd = accept( listen6_fd, &usa.sa, &sz ); }
	else if ( listen6s_fd != -1 && FD_ISSET( listen6s_fd, &lfdset ) ) { conn_fd = accept( listen6s_fd, &usa.sa, &sz ); }
	else
	    {
	    syslog( LOG_CRIT, "select failure" );
	    (void) fprintf( stderr, "%s: select failure\n", argv0 );
	    exit( 1 );
	    }
	if ( conn_fd < 0 )
	    {
	    if ( errno == EINTR || errno == EAGAIN )
		continue;	/* try again */
#ifdef EPROTO
	    if ( errno == EPROTO )
		continue;	/* try again */
#endif /* EPROTO */
	    syslog( LOG_CRIT, "accept - %m" );
	    perror( "accept" );
	    exit( 1 );
	    }

	/* Fork a sub-process to handle the connection. */
	r = fork();
	if ( r < 0 )
	    {
	    syslog( LOG_CRIT, "fork - %m" );
	    perror( "fork" );
	    exit( 1 );
	    }
	if ( r == 0 )
	    {
	    /* Child process. */
	    client_addr = usa;
	    if ( listen4_fd != -1 )  { (void) close( listen4_fd );  }
	    if ( listen4s_fd != -1 ) { (void) close( listen4s_fd ); }
	    if ( listen6_fd != -1 )  { (void) close( listen6_fd );  }
	    if ( listen6s_fd != -1 ) { (void) close( listen6s_fd ); }
 

	    handle_request(is_ssl, conn_port);
	    exit( 0 );
	    }
	(void) close( conn_fd );
	}
    }


static void
usage( void )
    {
	    /*add in gargoyle variables (at the end) here */
#ifdef HAVE_SSL
    (void) fprintf( stderr, "usage:  %s [-C configfile] [-D] [-S use ssl, if no ssl port is specified all connections will be SSL ] [-E certfile] [-SP ssl port ] [-Y cipher] [-p port ] [-d dir] [-dd data_dir] [-c cgipat] [-u user] [-h hostname] [-r] [-v] [-l logfile] [-i pidfile] [-T charset] [-P P3P] [-M maxage] [-DRN default realm name ] [-DRP default realm password file] [-DPF default page file] [-PNF Page to load when 404 Not Found error occurs] \n", argv0 );
#else /* HAVE_SSL */
    (void) fprintf( stderr, "usage:  %s [-C configfile] [-D] [-p port] [-d dir] [-dd data_dir] [-c cgipat] [-u user] [-h hostname] [-r] [-v] [-l logfile] [-i pidfile] [-T charset] [-P P3P] [-M maxage] [-DRN default realm name ] [-DRP default realm password file] [-DPF default page file] [-PNF Page to load when 404 Not Found error occurs]  \n", argv0 );
#endif /* HAVE_SSL */
    exit( 1 );
    }


static void
read_config( char* filename )
    {
    FILE* fp;
    char line[10000];
    char* cp;
    char* cp2;
    char* name;
    char* value;

    fp = fopen( filename, "r" );
    if ( fp == (FILE*) 0 )
	{
	syslog( LOG_CRIT, "%s - %m", filename );
	perror( filename );
	exit( 1 );
	}

    while ( fgets( line, sizeof(line), fp ) != (char*) 0 )
	{
	/* Trim comments. */
	if ( ( cp = strchr( line, '#' ) ) != (char*) 0 )
	    *cp = '\0';

	/* Skip leading whitespace. */
	cp = line;
	cp += strspn( cp, " \t\012\015" );

	/* Split line into words. */
	while ( *cp != '\0' )
	    {
	    /* Find next whitespace. */
	    cp2 = cp + strcspn( cp, " \t\012\015" );
	    /* Insert EOS and advance next-word pointer. */
	    while ( *cp2 == ' ' || *cp2 == '\t' || *cp2 == '\012' || *cp2 == '\015' )
		*cp2++ = '\0';
	    /* Split into name and value. */
	    name = cp;
	    value = strchr( name, '=' );
	    if ( value != (char*) 0 )
		*value++ = '\0';
	    /* Interpret. */
	    if ( strcasecmp( name, "debug" ) == 0 )
		{
		no_value_required( name, value );
		debug = 1;
		}
	    else if ( strcasecmp( name, "port" ) == 0 )
		{
		value_required( name, value );
		port = (unsigned short) atoi( value );
		}
	    else if ( strcasecmp( name, "dir" ) == 0 )
		{
		value_required( name, value );
		dir = e_strdup( value );
		}
	    else if ( strcasecmp( name, "data_dir" ) == 0 )
		{
		value_required( name, value );
		data_dir = e_strdup( value );
		}
	    else if ( strcasecmp( name, "chroot" ) == 0 )
		{
		no_value_required( name, value );
		do_chroot = 1;
		}
	    else if ( strcasecmp( name, "nochroot" ) == 0 )
		{
		no_value_required( name, value );
		do_chroot = 0;
		}
	    else if ( strcasecmp( name, "user" ) == 0 )
		{
		value_required( name, value );
		user = e_strdup( value );
		}
	    else if ( strcasecmp( name, "cgipat" ) == 0 )
		{
		value_required( name, value );
		cgi_pattern = e_strdup( value );
		}
	    else if ( strcasecmp( name, "urlpat" ) == 0 )
		{
		value_required( name, value );
		url_pattern = e_strdup( value );
		}
	    else if ( strcasecmp( name, "noemptyreferers" ) == 0 )
		{
		value_required( name, value );
		no_empty_referers = 1;
		}
	    else if ( strcasecmp( name, "localpat" ) == 0 )
		{
		value_required( name, value );
		local_pattern = e_strdup( value );
		}
	    else if ( strcasecmp( name, "host" ) == 0 )
		{
		value_required( name, value );
		hostname = e_strdup( value );
		}
	    else if ( strcasecmp( name, "logfile" ) == 0 )
		{
		value_required( name, value );
		logfile = e_strdup( value );
		}
	    else if ( strcasecmp( name, "vhost" ) == 0 )
		{
		no_value_required( name, value );
		vhost = 1;
		}
	    else if ( strcasecmp( name, "pidfile" ) == 0 )
		{
		value_required( name, value );
		pidfile = e_strdup( value );
		}
	    else if ( strcasecmp( name, "charset" ) == 0 )
		{
		value_required( name, value );
		charset = e_strdup( value );
		}
	    else if ( strcasecmp( name, "p3p" ) == 0 )
		{
		value_required( name, value );
		p3p = e_strdup( value );
		}
	    else if ( strcasecmp( name, "max_age" ) == 0 )
		{
		value_required( name, value );
		max_age = atoi( value );
		}
	    /* load gargoyle variables from config */
	    else if ( strcasecmp( name, "default_realm_name" ) == 0 )
		{
		value_required( name, value );
		defaultRealmName = e_strdup( value );
		}
	    else if ( strcasecmp( name, "default_realm_password_file" ) == 0 )
	    	{
		value_required( name, value );
		defaultRealmPasswordFile = e_strdup( value );
	    	}
	    else if ( strcasecmp( name, "default_page_file" ) == 0 )
	    	{
		value_required( name, value );
		defaultPageFile = e_strdup( value );
	    	}
	    else if( strcasecmp( name, "page_not_found_file" ) == 0 )
	    	{
		value_required( name, value );
		pageNotFoundFile = e_strdup( value );
	    	}
	    else if( strcasecmp( name, "allow_directory_listing" ) == 0 )
	  	{
			value_required( name, value );
			if(strcmp(value, "1") == 0 || strcmp(value, "true") == 0 || strcmp(value, "TRUE") == 0 || strcmp(value, "yes") == 0 || strcmp(value, "YES") == 0 )
			{
				allowDirectoryListing = 1;
			}
			else
			{
				allowDirectoryListing = 0;
			}
	    	}
     




	    /* end gargoyle variables */
#ifdef HAVE_SSL
	    else if ( strcasecmp( name, "ssl" ) == 0 )
		{
		no_value_required( name, value );
		do_ssl = 1;
		}
	    else if ( strcasecmp( name, "certfile" ) == 0 )
		{
		value_required( name, value );
		certfile = e_strdup( value );
		}
	    else if ( strcasecmp( name, "cipher" ) == 0 )
		{
		value_required( name, value );
		cipher = e_strdup( value );
		}
#endif /* HAVE_SSL */
	    else
		{
		(void) fprintf(
		    stderr, "%s: unknown config option '%s'\n", argv0, name );
		exit( 1 );
		}

	    /* Advance to next word. */
	    cp = cp2;
	    cp += strspn( cp, " \t\012\015" );
	    }
	}

    (void) fclose( fp );
    }


static void
value_required( char* name, char* value )
    {
    if ( value == (char*) 0 )
	{
	(void) fprintf(
	    stderr, "%s: value required for %s option\n", argv0, name );
	exit( 1 );
	}
    }


static void
no_value_required( char* name, char* value )
    {
    if ( value != (char*) 0 )
	{
	(void) fprintf(
	    stderr, "%s: no value required for %s option\n",
	    argv0, name );
	exit( 1 );
	}
    }


static int
initialize_listen_socket( usockaddr* usaP )
    {
    int listen_fd;
    int i;

    /* Check sockaddr. */
    if ( ! sockaddr_check( usaP ) )
	{
	syslog(
	    LOG_ERR, "unknown sockaddr family on listen socket - %d",
	    usaP->sa.sa_family );
	(void) fprintf(
	    stderr, "%s: unknown sockaddr family on listen socket - %d\n",
	    argv0, usaP->sa.sa_family );
	return -1;
	}

    listen_fd = socket( usaP->sa.sa_family, SOCK_STREAM, 0 );
    if ( listen_fd < 0 )
	{
		if( usaP->sa.sa_family == AF_INET6 && (errno == ENOPROTOOPT || errno == EPROTONOSUPPORT || errno == ESOCKTNOSUPPORT || EPFNOSUPPORT || EAFNOSUPPORT ))
		{
			/* IPV6 not compiled into kernel, no big deal, don't print errors */
		}
		else
		{
			syslog( LOG_CRIT, "socket %.80s - %m", ntoa( usaP ) );
			perror( "socket" );
		}
		return -1;
	}

    (void) fcntl( listen_fd, F_SETFD, 1 );

    i = 1;
    if ( setsockopt( listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*) &i, sizeof(i) ) < 0 )
	{
	syslog( LOG_CRIT, "setsockopt SO_REUSEADDR - %m" );
	perror( "setsockopt SO_REUSEADDR" );
	return -1;
	}

    if ( bind( listen_fd, &usaP->sa, sockaddr_len( usaP ) ) < 0 )
	{
	syslog( LOG_CRIT, "bind %.80s - %m", ntoa( usaP ) );
	perror( "bind" );
	return -1;
	}

    if ( listen( listen_fd, 1024 ) < 0 )
	{
	syslog( LOG_CRIT, "listen - %m" );
	perror( "listen" );
	return -1;
	}

#ifdef HAVE_ACCEPT_FILTERS
    {
    struct accept_filter_arg af;
    (void) bzero( &af, sizeof(af) );
    (void) strcpy( af.af_name, ACCEPT_FILTER_NAME );
    (void) setsockopt( listen_fd, SOL_SOCKET, SO_ACCEPTFILTER, (char*) &af, sizeof(af) );
    }
#endif /* HAVE_ACCEPT_FILTERS */

    return listen_fd;
    }


/* This runs in a child process, and exits when done, so cleanup is
 * not needed.
*/
static void handle_request( int is_ssl, unsigned short conn_port )
{
	char* method_str;
	char* line;
	char* cp;
	int r, file_len, i;
	const char* index_names[] = {"index.html", "index.htm", "index.xhtml", "index.xht", "Default.htm", "index.cgi" };

	/* Set up the timeout for reading. */
#ifdef HAVE_SIGSET
	(void) sigset( SIGALRM, handle_read_timeout_sig );
#else /* HAVE_SIGSET */
	(void) signal( SIGALRM, handle_read_timeout_sig );
#endif /* HAVE_SIGSET */
	(void) alarm( READ_TIMEOUT );

	/* Initialize the request variables. */
	remoteuser = (char*) 0;
	method = METHOD_UNKNOWN;
	path = (char*) 0;
	file = (char*) 0;
	pathinfo = (char*) 0;
	query = "";
	protocol = (char*) 0;
	status = 0;
	bytes = -1;
	req_hostname = (char*) 0;

	authorization = (char*) 0;
	content_type = (char*) 0;
	content_length = -1;
	cookie = (char*) 0;
	host = (char*) 0;
	if_modified_since = (time_t) -1;
	referer = "";
	useragent = "";

#ifdef TCP_NOPUSH
	/* Set the TCP_NOPUSH socket option, to try and avoid the 0.2 second
	** delay between sending the headers and sending the data.  A better
	** solution is writev() (as used in thttpd), or send the headers with
	** send(MSG_MORE) (only available in Linux so far).
	*/
	r = 1;
	(void) setsockopt(
	conn_fd, IPPROTO_TCP, TCP_NOPUSH, (void*) &r, sizeof(r) );
#endif /* TCP_NOPUSH */

#ifdef HAVE_SSL
	if ( is_ssl )
	{
		#ifdef USE_OPENSSL
			ssl = SSL_new( ssl_ctx );
			SSL_set_fd( ssl, conn_fd );
			if ( SSL_accept( ssl ) == 0 )
			{
				ERR_print_errors_fp( stderr );
				exit( 1 );
			}
		#endif
		#ifdef USE_CYASSL
			ssl = SSL_new( ssl_ctx );
			SSL_set_fd( ssl, conn_fd );
			int accept_ret = SSL_accept( ssl );
			if ( accept_ret <= 0 )
			{
				int e = SSL_get_error(ssl, accept_ret);
				/* 
				 * if SSL version mismatch it's not really a big deal, 
				 * client will just try again with right version, 
				 * so don't print an error in this case 
				 */
				if(e != VERSION_ERROR) 
				{
					syslog( LOG_CRIT, "error: can't initialize ssl connection, error = %d\n", e);
				}
				exit( 1 );
			}
		#endif

		#ifdef USE_MATRIXSSL
			ssl = SSL_new(keys, SSL_FLAGS_SERVER);
			SSL_set_fd( ssl, conn_fd );
			if ( SSL_accept( ssl ) <= 0 )
			{
				perror( "SSL_accept" );
			}
		#endif
	}
#endif 

	/* Read in the request. */
	start_request();
	for (;;)
	{
		char buf[10000];
		int r = my_read( buf, sizeof(buf), is_ssl );
		if ( r < 0 && ( errno == EINTR || errno == EAGAIN ) )
		{
			continue;
		}
		if ( r <= 0 )
		{
			break;
		}
		(void) alarm( READ_TIMEOUT );
		add_to_request( buf, r );
		if ( strstr( request, "\015\012\015\012" ) != (char*) 0 || strstr( request, "\012\012" ) != (char*) 0 )
		{
			break;
		}
	}

	/* Parse the first line of the request. */
	method_str = get_request_line();
	if ( method_str == (char*) 0 )
	{
		send_error( 400, "Bad Request", "", "Can't parse request.", is_ssl );
	}
	path = strpbrk( method_str, " \t\012\015" );
	if ( path == (char*) 0 )
	{
		send_error( 400, "Bad Request", "", "Can't parse request.", is_ssl );
	}
	*path++ = '\0';
	path += strspn( path, " \t\012\015" );
	protocol = strpbrk( path, " \t\012\015" );
	if ( protocol == (char*) 0 )
	{
		send_error( 400, "Bad Request", "", "Can't parse request.", is_ssl );
	}
	*protocol++ = '\0';
	protocol += strspn( protocol, " \t\012\015" );
	query = strchr( path, '?' );
	
	
	
	if ( query == (char*) 0 )
	{
		query = "";
	}
	else
	{
		*query++ = '\0';
	}

	/* Parse the rest of the request headers. */
	while ( ( line = get_request_line() ) != (char*) 0 )
	{
		if ( line[0] == '\0' )
		{
			break;
		}
		else if ( strncasecmp( line, "Authorization:", 14 ) == 0 )
		{
			cp = &line[14];
			cp += strspn( cp, " \t" );
			authorization = cp;
		}
		else if ( strncasecmp( line, "Content-Length:", 15 ) == 0 )
		{
			cp = &line[15];
			cp += strspn( cp, " \t" );
			content_length = atol( cp );
		}
		else if ( strncasecmp( line, "Content-Type:", 13 ) == 0 )
		{
			cp = &line[13];
			cp += strspn( cp, " \t" );
			content_type = cp;
		}
		else if ( strncasecmp( line, "Cookie:", 7 ) == 0 )
		{
			cp = &line[7];
			cp += strspn( cp, " \t" );
			cookie = cp;
		}
		else if ( strncasecmp( line, "Host:", 5 ) == 0 )
		{
			cp = &line[5];
			cp += strspn( cp, " \t" );
			host = cp;
			if ( strchr( host, '/' ) != (char*) 0 || host[0] == '.' )
			{
				send_error( 400, "Bad Request", "", "Can't parse request.", is_ssl );
			}
		}
		else if ( strncasecmp( line, "If-Modified-Since:", 18 ) == 0 )
		{
			cp = &line[18];
			cp += strspn( cp, " \t" );
			if_modified_since = tdate_parse( cp );
		}
		else if ( strncasecmp( line, "Referer:", 8 ) == 0 )
		{
			cp = &line[8];
			cp += strspn( cp, " \t" );
			referer = cp;
		}
		else if ( strncasecmp( line, "User-Agent:", 11 ) == 0 )
		{
			cp = &line[11];
			cp += strspn( cp, " \t" );
			useragent = cp;
		}
	}

	if ( strcasecmp( method_str, get_method_str( METHOD_GET ) ) == 0 )
	{
		method = METHOD_GET;
	}
	else if ( strcasecmp( method_str, get_method_str( METHOD_HEAD ) ) == 0 )
	{
		method = METHOD_HEAD;
	}
	else if ( strcasecmp( method_str, get_method_str( METHOD_POST ) ) == 0 )
	{
		method = METHOD_POST;
	}
	else
	{
		send_error( 501, "Not Implemented", "", "That method is not implemented.", is_ssl );
	}
	strdecode( path, path );
	if ( path[0] != '/' )
	{
		send_error( 400, "Bad Request", "", "Bad filename.", is_ssl );
	}
	file = &(path[1]);
	de_dotdot( file );
	if ( file[0] == '\0' )
	{
		file = "./";
	}
	if ( file[0] == '/' || ( file[0] == '.' && file[1] == '.' && (file[2] == '\0' || file[2] == '/') ) )
	{
		send_error( 400, "Bad Request", "", "Illegal filename.", is_ssl );
	}
	if ( vhost )
	{
		file = virtual_file( file );
	}

	/* Set up the timeout for writing. */
#ifdef HAVE_SIGSET
	(void) sigset( SIGALRM, handle_write_timeout );
#else /* HAVE_SIGSET */
	(void) signal( SIGALRM, handle_write_timeout );
#endif /* HAVE_SIGSET */
	(void) alarm( WRITE_TIMEOUT );

	r = stat( file, &sb );
	if ( r < 0 )
	{
		r = get_pathinfo();
	}
	if ( r < 0 )
	{
		/*
		FILE* f = fopen("/tmp/test1.tmp", "w");
		fprintf(f, "not found occured, file=%s\n", file);
		fclose(f);
		*/

		if(pageNotFoundFile != NULL && host != NULL)
		{
			/*
			f = fopen("/tmp/test2.tmp", "w");
			fprintf(f, "not found occured, have pageNotFoundFile = %s\n", pageNotFoundFile);
			fclose(f);
			*/
			/*
			file=pageNotFoundFile;
			r = stat( file, &sb );
			if ( r < 0 )
			{
				r = get_pathinfo();
			}
			*/
			send_redirect("", host, pageNotFoundFile, is_ssl);
		}
		else
		{
			send_error( 404, "Not Found", "", "File not found.", is_ssl );
		}
		
	}


	file_len = strlen( file );
	if ( ! S_ISDIR( sb.st_mode ) )
	{
		/* Not a directory. */
		while ( file[file_len - 1] == '/' )
		{
			file[file_len - 1] = '\0';
			--file_len;
		}
		do_file(is_ssl, conn_port);
	}
	else
	{
		char idx[10000];
		unsigned char found_index = 0;
		/* The filename is a directory.  Is it missing the trailing slash? */
		if ( file[file_len - 1] != '/' && pathinfo == (char*) 0 )
		{
			char location[10000];
			if ( query[0] != '\0' )
			{
				(void) snprintf(location, sizeof(location), "Location: %s/?%s", path, query );
			}
			else
			{
				(void) snprintf( location, sizeof(location), "Location: %s/", path );
				send_error( 302, "Found", location, "Directories must end with a slash.", is_ssl );
			}
		}

		/* Check for an index file.  */
		/* This section has been modified to accomodate defaultPageFile variable for gargoyle*/
		if(defaultPageFile != NULL)
		{
			(void) snprintf( idx, sizeof(idx), "%s%s", file, defaultPageFile );
			if ( stat( idx, &sb ) >= 0 )
			{
				file = idx;
				do_file(is_ssl, conn_port);
				found_index = 1;
			}
		}
		for ( i = 0; i < (sizeof(index_names) / sizeof(char*)) && found_index == 0; ++i )
		{
			(void) snprintf( idx, sizeof(idx), "%s%s", file, index_names[i] );
			if ( stat( idx, &sb ) >= 0 )
			{
				file = idx;
				do_file(is_ssl, conn_port);
				found_index = 1;
			}
		}



		if(found_index == 0)
		{
			/* Nope, no index file, so it's an actual directory request. */
			if(allowDirectoryListing == 0)
			{
				if(pageNotFoundFile != NULL && host != NULL)
				{
					send_redirect("", host, pageNotFoundFile, is_ssl);
				}
				else
				{
					send_error( 404, "Not Found", "", "File not found.", is_ssl );
				}
			}
			else
			{
				do_dir(is_ssl);
			}
		}
		/* end gargoyle modifications */
	}

#ifdef HAVE_SSL
	if(is_ssl)
	{
		SSL_free( ssl );
	}
#endif /* HAVE_SSL */

}


static void
de_dotdot( char* file )
    {
    char* cp;
    char* cp2;
    int l;

    /* Collapse any multiple / sequences. */
    while ( ( cp = strstr( file, "//") ) != (char*) 0 )
	{
	for ( cp2 = cp + 2; *cp2 == '/'; ++cp2 )
	    continue;
	(void) strcpy( cp + 1, cp2 );
	}

    /* Remove leading ./ and any /./ sequences. */
    while ( strncmp( file, "./", 2 ) == 0 )
	(void) strcpy( file, file + 2 );
    while ( ( cp = strstr( file, "/./") ) != (char*) 0 )
	(void) strcpy( cp, cp + 2 );

    /* Alternate between removing leading ../ and removing xxx/../ */
    for (;;)
	{
	while ( strncmp( file, "../", 3 ) == 0 )
	    (void) strcpy( file, file + 3 );
	cp = strstr( file, "/../" );
	if ( cp == (char*) 0 )
	    break;
	for ( cp2 = cp - 1; cp2 >= file && *cp2 != '/'; --cp2 )
	    continue;
	(void) strcpy( cp2 + 1, cp + 4 );
	}

    /* Also elide any xxx/.. at the end. */
    while ( ( l = strlen( file ) ) > 3 &&
	    strcmp( ( cp = file + l - 3 ), "/.." ) == 0 )
	{
	for ( cp2 = cp - 1; cp2 >= file && *cp2 != '/'; --cp2 )
	    continue;
	if ( cp2 < file )
	    break;
	*cp2 = '\0';
	}
    }


static int
get_pathinfo( void )
    {
    int r;

    pathinfo = &file[strlen(file)];
    for (;;)
	{
	do
	    {
	    --pathinfo;
	    if ( pathinfo <= file )
		{
		pathinfo = (char*) 0;
		return -1;
		}
	    }
	while ( *pathinfo != '/' );
	*pathinfo = '\0';
	r = stat( file, &sb );
	if ( r >= 0 )
	    {
	    ++pathinfo;
	    return r;
	    }
	else
	    *pathinfo = '/';
	}
    }


static void do_file( int is_ssl, unsigned short conn_port )
{
	char buf[10000];
	char mime_encodings[500];
	const char* mime_type;
	char fixed_mime_type[500];
	char* cp;
	int fd;

	/* Check authorization for this directory. */
	(void) strncpy( buf, file, sizeof(buf) );
	cp = strrchr( buf, '/' );
	if ( cp == (char*) 0 )
	{
		(void) strcpy( buf, "." );
	}
	else
	{
		*cp = '\0';
	}
	auth_check( buf, is_ssl);

	/* Check if the filename is the AUTH_FILE itself - that's verboten. */
	if ( strcmp( file, AUTH_FILE ) == 0 || ( strcmp( &(file[strlen(file) - sizeof(AUTH_FILE) + 1]), AUTH_FILE ) == 0 && file[strlen(file) - sizeof(AUTH_FILE)] == '/' ) )
	{
		syslog( LOG_NOTICE, "%.80s URL \"%.80s\" tried to retrieve an auth file", ntoa( &client_addr ), path );
		send_error( 403, "Forbidden", "", "File is protected.", is_ssl );
	}

	/* gargoyle addition -- filename shouldn't be default password file either */
	if(defaultRealmName != NULL && defaultRealmPasswordFile != NULL)
	{
		if (	strcmp( file, defaultRealmPasswordFile ) == 0 || strcmp( &(file[strlen(file) - sizeof(defaultRealmPasswordFile) + 1]), defaultRealmPasswordFile ) == 0 && file[strlen(file) - sizeof(defaultRealmPasswordFile)] == '/' )
		{
			syslog(	LOG_NOTICE, "%.80s URL \"%.80s\" tried to retrieve an auth file", ntoa( &client_addr ), path );
			send_error( 403, "Forbidden", "", "File is protected.", is_ssl );
		}
	}
	/* End gargoyle addition */


	/* Referer check. */
	check_referer(is_ssl);

	/* Is it CGI? */
	if ( cgi_pattern != (char*) 0 && match( cgi_pattern, file ) )
	{
		do_cgi(is_ssl, conn_port);
		return;
	}
	else if ( pathinfo != (char*) 0 )
	{
		send_error( 404, "Not Found", "", "File not found.", is_ssl );
	}

	fd = open( file, O_RDONLY );
	if ( fd < 0 )
	{
		syslog( LOG_INFO, "%.80s File \"%.80s\" is protected", ntoa( &client_addr ), path );
		send_error( 403, "Forbidden", "", "File is protected.", is_ssl );
	}
	mime_type = figure_mime( file, mime_encodings, sizeof(mime_encodings) );
	(void) snprintf( fixed_mime_type, sizeof(fixed_mime_type), mime_type, charset );
	if ( if_modified_since != (time_t) -1 && if_modified_since >= sb.st_mtime )
	{
		add_headers(304, "Not Modified", "", mime_encodings, fixed_mime_type, (off_t) -1, sb.st_mtime );
		send_response(is_ssl);
		return;
	}
	add_headers(200, "Ok", "", mime_encodings, fixed_mime_type, sb.st_size, sb.st_mtime );
	send_response(is_ssl);
	if ( method == METHOD_HEAD )
	{
		return;
	}

	if ( sb.st_size > 0 )	/* ignore zero-length files */
	{
#ifdef HAVE_SENDFILE

#ifndef HAVE_SSL
		(void) my_sendfile( fd, conn_fd, 0, sb.st_size );
#else /* HAVE_SSL */
		if ( is_ssl )
		{
			send_via_write( fd, sb.st_size, is_ssl );
		}
		else
		{
			(void) my_sendfile( fd, conn_fd, 0, sb.st_size );
		}
#endif /* HAVE_SSL */

#else /* HAVE_SENDFILE */
		send_via_write( fd, sb.st_size, is_ssl );

#endif /* HAVE_SENDFILE */
	}

	(void) close( fd );
}


static void do_dir( int is_ssl )
{
	char buf[10000];
	size_t buflen;
	char* contents;
	size_t contents_size, contents_len;
#ifdef HAVE_SCANDIR
	int n, i;
	struct dirent **dl;
	char* name_info;
#else /* HAVE_SCANDIR */
	char command[10000];
	FILE* fp;
#endif /* HAVE_SCANDIR */

	if ( pathinfo != (char*) 0 ) { send_error( 404, "Not Found", "", "File not found.", is_ssl ); }

	/* Check authorization for this directory. */
	auth_check( file, is_ssl );

	/* Referer check. */
	check_referer(is_ssl);

#ifdef HAVE_SCANDIR
	n = scandir( file, &dl, NULL, alphasort );
	if ( n < 0 )
	{
		syslog( LOG_INFO, "%.80s Directory \"%.80s\" is protected", ntoa( &client_addr ), path );
		send_error( 403, "Forbidden", "", "Directory is protected.", is_ssl );
	}
#endif /* HAVE_SCANDIR */

	contents_size = 0;
	buflen = snprintf( buf, sizeof(buf), "<HTML>\n<HEAD><TITLE>Index of %s</TITLE></HEAD>\n<BODY BGCOLOR=\"#99cc99\" TEXT=\"#000000\" LINK=\"#2020ff\" VLINK=\"#4040cc\">\n<H4>Index of %s</H4>\n<PRE>\n", file, file );
	add_to_buf( &contents, &contents_size, &contents_len, buf, buflen );

#ifdef HAVE_SCANDIR

	for ( i = 0; i < n; ++i )
	{
		name_info = file_details( file, dl[i]->d_name );
		add_to_buf( &contents, &contents_size, &contents_len, name_info, strlen( name_info ) );
	}

#else /* HAVE_SCANDIR */
	/* Magic HTML ls command! */
	if ( strchr( file, '\'' ) == (char*) 0 )
	{
		(void) snprintf(
			command, sizeof(command),
			"ls -lgF '%s' | tail +2 | sed -e 's/^\\([^ ][^ ]*\\)\\(  *[^ ][^ ]*  *[^ ][^ ]*  *[^ ][^ ]*\\)\\(  *[^ ][^ ]*\\)  *\\([^ ][^ ]*  *[^ ][^ ]*  *[^ ][^ ]*\\)  *\\(.*\\)$/\\1 \\3  \\4  |\\5/' -e '/ -> /!s,|\\([^*]*\\)$,|<A HREF=\"\\1\">\\1</A>,' -e '/ -> /!s,|\\(.*\\)\\([*]\\)$,|<A HREF=\"\\1\">\\1</A>\\2,' -e '/ -> /s,|\\([^@]*\\)\\(@* -> \\),|<A HREF=\"\\1\">\\1</A>\\2,' -e 's/|//'",
			file );
		fp = popen( command, "r" );
		for (;;)
		{
			size_t r;
			r = fread( buf, 1, sizeof(buf), fp );
			if ( r == 0 )
			break;
			add_to_buf( &contents, &contents_size, &contents_len, buf, r );
		}
		(void) pclose( fp );
	}
#endif /* HAVE_SCANDIR */

	buflen = snprintf( buf, sizeof(buf), "</PRE>\n<HR>\n<ADDRESS><A HREF=\"%s\">%s</A></ADDRESS>\n</BODY>\n</HTML>\n", SERVER_URL, SERVER_SOFTWARE );
	add_to_buf( &contents, &contents_size, &contents_len, buf, buflen );

	
	add_headers( 200, "Ok", "", "", default_content_type, contents_len, sb.st_mtime );
	
	if ( method != METHOD_HEAD )
	{
		add_to_response( contents, contents_len );
	}
	send_response(is_ssl);
}


#ifdef HAVE_SCANDIR

static char*
file_details( const char* dir, const char* name )
    {
    struct stat sb;
    char f_time[20];
    static char encname[1000];
    static char buf[2000];

    (void) snprintf( buf, sizeof(buf), "%s/%s", dir, name );
    if ( lstat( buf, &sb ) < 0 )
	return "???";
    (void) strftime( f_time, sizeof( f_time ), "%d%b%Y %H:%M", localtime( &sb.st_mtime ) );
    strencode( encname, sizeof(encname), name );
    (void) snprintf(
	buf, sizeof( buf ), "<A HREF=\"%s\">%-32.32s</A>    %15s %14lld\n",
	encname, name, f_time, (long long int) sb.st_size );
    return buf;
    }


/* Copies and encodes a string. */
static void
strencode( char* to, size_t tosize, const char* from )
    {
    int tolen;

    for ( tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from )
	{
	if ( isalnum(*from) || strchr( "/_.-~", *from ) != (char*) 0 )
	    {
	    *to = *from;
	    ++to;
	    ++tolen;
	    }
	else
	    {
	    (void) sprintf( to, "%%%02x", (int) *from & 0xff );
	    to += 3;
	    tolen += 3;
	    }
	}
    *to = '\0';
    }

#endif /* HAVE_SCANDIR */


static void
do_cgi( int is_ssl, unsigned short conn_port )
    {
    char** argp;
    char** envp;
    int parse_headers;
    char* binary;
    char* directory;

    if ( method != METHOD_GET && method != METHOD_POST )
	send_error( 501, "Not Implemented", "", "That method is not implemented for CGI.", is_ssl );

    /* If the socket happens to be using one of the stdin/stdout/stderr
    ** descriptors, move it to another descriptor so that the dup2 calls
    ** below don't screw things up.  We arbitrarily pick fd 3 - if there
    ** was already something on it, we clobber it, but that doesn't matter
    ** since at this point the only fd of interest is the connection.
    ** All others will be closed on exec.
    */
    if ( conn_fd == STDIN_FILENO || conn_fd == STDOUT_FILENO || conn_fd == STDERR_FILENO )
	{
	int newfd = dup2( conn_fd, STDERR_FILENO + 1 );
	if ( newfd >= 0 )
	    conn_fd = newfd;
	/* If the dup2 fails, shrug.  We'll just take our chances.
	** Shouldn't happen though.
	*/
	}

    /* Make the environment vector. */
    envp = make_envp(is_ssl, conn_port);

    /* Make the argument vector. */
    argp = make_argp();

    /* Set up stdin.  For POSTs we may have to set up a pipe from an
    ** interposer process, depending on if we've read some of the data
    ** into our buffer.  We also have to do this for all SSL CGIs.
    */
#ifdef HAVE_SSL
    if ( ( method == METHOD_POST && request_len > request_idx ) || is_ssl )
#else /* HAVE_SSL */
    if ( ( method == METHOD_POST && request_len > request_idx ) )
#endif /* HAVE_SSL */
	{
	int p[2];
	int r;

	if ( pipe( p ) < 0 )
	    send_error( 500, "Internal Error", "", "Something unexpected went wrong making a pipe.", is_ssl );
	r = fork();
	if ( r < 0 )
	    send_error( 500, "Internal Error", "", "Something unexpected went wrong forking an interposer.", is_ssl );
	if ( r == 0 )
	    {
	    /* Interposer process. */
	    (void) close( p[0] );
	    cgi_interpose_input( p[1], is_ssl );
	    exit( 0 );
	    }
	(void) close( p[1] );
	if ( p[0] != STDIN_FILENO )
	    {
	    (void) dup2( p[0], STDIN_FILENO );
	    (void) close( p[0] );
	    }
	}
    else
	{
	/* Otherwise, the request socket is stdin. */
	if ( conn_fd != STDIN_FILENO )
	    (void) dup2( conn_fd, STDIN_FILENO );
	}

    /* Set up stdout/stderr.  For SSL, or if we're doing CGI header parsing,
    ** we need an output interposer too.
    */
    if ( strncmp( argp[0], "nph-", 4 ) == 0 )
	parse_headers = 0;
    else
	parse_headers = 1;
#ifdef HAVE_SSL
    if ( parse_headers || is_ssl )
#else /* HAVE_SSL */
    if ( parse_headers )
#endif /* HAVE_SSL */
	{
	int p[2];
	int r;

	if ( pipe( p ) < 0 )
	    send_error( 500, "Internal Error", "", "Something unexpected went wrong making a pipe.", is_ssl );
	r = fork();
	if ( r < 0 )
	    send_error( 500, "Internal Error", "", "Something unexpected went wrong forking an interposer.", is_ssl );
	if ( r == 0 )
	    {
	    /* Interposer process. */
	    (void) close( p[1] );
	    cgi_interpose_output( p[0], parse_headers, is_ssl );
	    exit( 0 );
	    }
	(void) close( p[0] );
	if ( p[1] != STDOUT_FILENO )
	    (void) dup2( p[1], STDOUT_FILENO );
	if ( p[1] != STDERR_FILENO )
	    (void) dup2( p[1], STDERR_FILENO );
	if ( p[1] != STDOUT_FILENO && p[1] != STDERR_FILENO )
	    (void) close( p[1] );
	}
    else
	{
	/* Otherwise, the request socket is stdout/stderr. */
	if ( conn_fd != STDOUT_FILENO )
	    (void) dup2( conn_fd, STDOUT_FILENO );
	if ( conn_fd != STDERR_FILENO )
	    (void) dup2( conn_fd, STDERR_FILENO );
	}

    /* At this point we would like to set conn_fd to be close-on-exec.
    ** Unfortunately there seems to be a Linux problem here - if we
    ** do this close-on-exec in Linux, the socket stays open but stderr
    ** gets closed - the last fd duped from the socket.  What a mess.
    ** So we'll just leave the socket as is, which under other OSs means
    ** an extra file descriptor gets passed to the child process.  Since
    ** the child probably already has that file open via stdin stdout
    ** and/or stderr, this is not a problem.
    */
    /* (void) fcntl( conn_fd, F_SETFD, 1 ); */

    /* Close the log file. */
    if ( logfp != (FILE*) 0 )
	(void) fclose( logfp );

    /* Close syslog. */
    closelog();

    /* Set priority. */
    if( nice( CGI_NICE ) < 0) { ; }

    /* Split the program into directory and binary, so we can chdir()
    ** to the program's own directory.  This isn't in the CGI 1.1
    ** spec, but it's what other HTTP servers do.
    */
    directory = e_strdup( file );
    binary = strrchr( directory, '/' );
    if ( binary == (char*) 0 )
	binary = file;
    else
	{
	*binary++ = '\0';
	if(chdir( directory ) < 0) { ; }	/* ignore errors */
	}

    /* Default behavior for SIGPIPE. */
#ifdef HAVE_SIGSET
    (void) sigset( SIGPIPE, SIG_DFL );
#else /* HAVE_SIGSET */
    (void) signal( SIGPIPE, SIG_DFL );
#endif /* HAVE_SIGSET */

    /* Run the program. */
    (void) execve( binary, argp, envp );

    /* Something went wrong. */
    send_error( 500, "Internal Error", "", "Something unexpected went wrong running a CGI program.", is_ssl );
    }


/* This routine is used only for POST requests.  It reads the data
 * from the request and sends it to the child process.  The only reason
 * we need to do it this way instead of just letting the child read
 * directly is that we have already read part of the data into our
 * buffer.
 *
 * Oh, and it's also used for all SSL CGIs.
*/
static void
cgi_interpose_input( int wfd, int is_ssl )
    {
    size_t c;
    ssize_t r, r2;
    char buf[1024];

    c = request_len - request_idx;
    if ( c > 0 )
	{
	if ( write( wfd, &(request[request_idx]), c ) != c )
	    return;
	}
    while ( c < content_length )
	{
	r = my_read( buf, MIN( sizeof(buf), content_length - c ), is_ssl );
	if ( r < 0 && ( errno == EINTR || errno == EAGAIN ) )
	    {
	    sleep( 1 );
	    continue;
	    }
	if ( r <= 0 )
	    return;
	for (;;)
	    {
	    r2 = write( wfd, buf, r );
	    if ( r2 < 0 && ( errno == EINTR || errno == EAGAIN ) )
		{
		sleep( 1 );
		continue;
		}
	    if ( r2 != r )
		return;
	    break;
	    }
	c += r;
	}
    post_post_garbage_hack(is_ssl);
    }


/* Special hack to deal with broken browsers that send a LF or CRLF
 * after POST data, causing TCP resets - we just read and discard up
 * to 2 bytes.  Unfortunately this doesn't fix the problem for CGIs
 * which avoid the interposer process due to their POST data being
 * short.  Creating an interposer process for all POST CGIs is
 * unacceptably expensive.
*/
static void
post_post_garbage_hack( int is_ssl )
    {
    char buf[2];

#ifdef HAVE_SSL
    if ( is_ssl )
	/* We don't need to do this for SSL, since the garbage has
	** already been read.  Probably.
	*/
	return;
#endif /* HAVE_SSL */

    set_ndelay( conn_fd );
    if( read( conn_fd, buf, sizeof(buf) ) < 0) { ; };
    clear_ndelay( conn_fd );
    }


/* This routine is used for parsed-header CGIs and for all SSL CGIs. */
static void cgi_interpose_output( int rfd, int parse_headers, int is_ssl )
{
	ssize_t r, r2;
	char buf[1024];
	
	if ( ! parse_headers )
	{
		/* If we're not parsing headers, write out the default status line
		 * and proceed to the echo phase.
		 */
		char http_head[] = "HTTP/1.0 200 OK\015\012";
		(void) my_write( http_head, sizeof(http_head), is_ssl );
	}
	else
	{
		/* Header parsing.  The idea here is that the CGI can return special
		 * headers such as "Status:" and "Location:" which change the return
		 * status of the response.  Since the return status has to be the very
		 * first line written out, we have to accumulate all the headers
		 * and check for the special ones before writing the status.  Then
		 * we write out the saved headers and proceed to echo the rest of
		 * the response.
		 */
		size_t headers_size, headers_len;
		char* headers;
		char* br;
		int status;
		char* title;
		char* cp;
		char* tmp_headers;
		int eof_after_headers;

	
		/* Slurp in all headers. */
		headers_size = 0;
		eof_after_headers = 0;
		add_to_buf( &headers, &headers_size, &headers_len, (char*) 0, 0 );
		for (;;)
		{
			r = read( rfd, buf, sizeof(buf) );
			if ( r < 0 && ( errno == EINTR || errno == EAGAIN ) )
			{
				sleep( 1 );
				continue;
			}
			if ( r <= 0 )
			{
				br = &(headers[headers_len]);
				eof_after_headers = 1;
				break;
			}
			add_to_buf( &headers, &headers_size, &headers_len, buf, r );
			if ( ( br = strstr( headers, "\015\012\015\012" ) ) != (char*) 0 || ( br = strstr( headers, "\012\012" ) ) != (char*) 0 )
			{
				break;
			}
		}

		/* NULL terminate the headers */
		tmp_headers = (char*)e_malloc(headers_len+1);
		memcpy(tmp_headers, headers, headers_len);
		tmp_headers[headers_len] = '\0';
		free(headers);
		headers=tmp_headers;
		

	
		/* If there were no headers, bail. */
		if ( headers[0] == '\0' )
		{
			return;
		}
		


		/* Figure out the status. */
		status = 200;
		char line[200];
		char* location = NULL;
		line[0] = '\0';
		if ( ( cp = strstr( headers, "Status:" ) ) != (char*) 0 && cp < br && ( cp == headers || *(cp-1) == '\012' ) )
		{
			cp += 7;
			cp += strspn( cp, " \t" );
			status = atoi( cp );
		}

		if(strstr(headers, "HTTP/") == headers)
		{
			char* e1 = strchr(headers, '\n');
			char* e2 = strchr(headers, '\r');
			char* s1 = strchr(headers, ' ');
			char* s2 = strchr(headers, '\t');
			char* end_first_line   = e1 >= headers && (e2 < headers || e1 < e2) ? e1 : e2;
			char* start_first_line = s1 >= headers && (s2 < headers || s1 < s2) ? s1 : s2;
			if(start_first_line >= headers && end_first_line > start_first_line)
			{
				*end_first_line = '\0';
				sprintf(line, "HTTP/1.0 %s\015\012", start_first_line+1);
				headers = end_first_line + 1;
				while(headers[0] == '\015' || headers[0] == '\012')
				{
					headers = headers + 1;
				}
			}
		}
		
		if ( ( cp = strstr( headers, "Location:" ) ) != (char*) 0 && cp < br && ( cp == headers || *(cp-1) == '\012' ) )
		{
			status = 302; //unless set to something else in initial line above
			char* location_start = strstr( headers, "Location:" );
			char* e1 = strchr(location_start, '\n');
			char* e2 = strchr(location_start, '\r');
			char* s1 = strchr(location_start, ' ');
			char* s2 = strchr(location_start, '\t');
			char* url_end   = e1 >= location_start && (e2 < location_start || e1 < e2) ? e1 : e2;
			char* url_start = s1 >= location_start && (s2 < location_start || s1 < s2) ? s1 : s2;
			if(url_start >= location_start && url_end > url_start)
			{
				char* original_url;
				while(*url_start == ' ' || *url_start == '\t')
				{
					url_start++;
				}
				*url_end = '\0';
				original_url = e_strdup(url_start);

				*url_end = '\015';
				while(*url_end == '\015' || *url_end == '\012')
				{
					url_end++;
				}


				//write over location, since we are modifying it and handling it separately
				while(*url_end != '\0')
				{
					location_start[0] = url_end[0];
					url_end++;
					location_start++;
				}
				location_start[0] = '\0';
				
				
				/*
				 * If a redirect Location header path doesn't start with either a protocol 
				 * or the root (e.g. "location.sh" instead of "http://1.2.3.4/location.sh" or "/location.sh")
				 * then Chrome (but no other browser) has a pissy little shit-fit, responding with
				 * the ever-so relevant, descriptive and helpful error message: ERR_RESPONSE_HEADERS_TRUNCATED
				*/
				if(strstr(original_url, "://") == NULL && original_url[0] != '/')
				{
					location = (char*)e_malloc(2+strlen(original_url));
					sprintf(location, "/%s", original_url);
				}
				else
				{
					location = e_strdup(original_url);
				}
				free(original_url);
			}
		}
		
		/* Write the status line. */
		switch ( status )
			{
			case 200: title = "OK"; break;
			case 302: title = "Found"; break;
			case 304: title = "Not Modified"; break;
			case 400: title = "Bad Request"; break;
			case 401: title = "Unauthorized"; break;
			case 403: title = "Forbidden"; break;
			case 404: title = "Not Found"; break;
			case 408: title = "Request Timeout"; break;
			case 500: title = "Internal Error"; break;
			case 501: title = "Not Implemented"; break;
			case 503: title = "Service Temporarily Overloaded"; break;
			default: title = "Something"; break;
			}
		if(line[0] == '\0')
		{
			(void) snprintf( buf, sizeof(buf), "HTTP/1.0 %d %s\015\012", status, title );
		}
		else
		{
			(void) snprintf( buf, sizeof(buf), "%s", line );
		}
		(void) my_write( buf, strlen( buf ), is_ssl );
		if(location != NULL)
		{
			sprintf(line, "Location: %s\015\012", location );
			(void) my_write(line, strlen(line), is_ssl );
			free(location);
		}
		if(strstr(headers, "Server:") == NULL)
		{
			sprintf(line, "Server: %s\015\012", SERVER_SOFTWARE );
			(void) my_write(line, strlen(line), is_ssl );
		}

		if(strstr(headers, "Date:") == NULL)
		{
			char timebuf[100];
			time_t now = time( (time_t*) 0 );
			const char* rfc1123_fmt = "%a, %d %b %Y %H:%M:%S GMT";
			(void) strftime( timebuf, sizeof(timebuf), rfc1123_fmt, gmtime( &now ) );
			sprintf(line, "Date: %s\015\012", timebuf);
			(void) my_write(line, strlen(line), is_ssl );
			if(strstr(headers, "Expires:") == NULL)
			{
				sprintf(line, "Expires: %s\015\012", timebuf);
				(void) my_write(line, strlen(line), is_ssl );
			}
			
		}
		else if(strstr(headers, "Expires:") == NULL)
		{
			char* exp_line = "Expires: Thu, 01 Jan 1970 00:00:00 GMT\015\012";
			(void) my_write(exp_line, strlen(exp_line), is_ssl );
		}
		if(strstr(headers, "Content-Type:") == NULL)
		{
			sprintf(line, "Content-Type: text/html; charset=%s\015\012", charset);;
			(void) my_write(line, strlen(line), is_ssl );
		}

	
		/* Write the saved headers. */
		(void) my_write( headers, strlen(headers), is_ssl );
		if(eof_after_headers == 1)
		{
			(void) my_write("\015\012", 2, is_ssl);
		}
	}
	
	/* Echo the rest of the output. */
	for (;;)
	{
		r = read( rfd, buf, sizeof(buf) );
		if ( r < 0 && ( errno == EINTR || errno == EAGAIN ) )
		{
			sleep( 1 );
			continue;
		}
		if ( r <= 0 )
		{
			goto done;
		}
		for (;;)
		{
			r2 = my_write( buf, r, is_ssl );
			if ( r2 < 0 && ( errno == EINTR || errno == EAGAIN ) )
			{
			sleep( 1 );
			continue;
			}
			if ( r2 != r )
			goto done;
			break;
		}
	}
	done:
	shutdown( conn_fd, SHUT_WR );
}


/* Set up CGI argument vector.  We don't have to worry about freeing
 * stuff since we're a sub-process.  This gets done after make_envp() because
 * we scribble on query.
*/
static char**
make_argp( void )
    {
    char** argp;
    int argn;
    char* cp1;
    char* cp2;

    /* By allocating an arg slot for every character in the query, plus
    ** one for the filename and one for the NULL, we are guaranteed to
    ** have enough.  We could actually use strlen/2.
    */
    argp = (char**) malloc( ( strlen( query ) + 2 ) * sizeof(char*) );
    if ( argp == NULL )
	return NULL;

    argp[0] = strrchr( file, '/' );
    if ( argp[0] != NULL )
	++argp[0];
    else
	argp[0] = file;

    argn = 1;
    /* According to the CGI spec at http://hoohoo.ncsa.uiuc.edu/cgi/cl.html,
    ** "The server should search the query information for a non-encoded =
    ** character to determine if the command line is to be used, if it finds
    ** one, the command line is not to be used."
    */
    if ( strchr( query, '=' ) == NULL )
	{
	for ( cp1 = cp2 = query; *cp2 != '\0'; ++cp2 )
	    {
	    if ( *cp2 == '+' )
		{
		*cp2 = '\0';
		strdecode( cp1, cp1 );
		argp[argn++] = cp1;
		cp1 = cp2 + 1;
		}
	    }
	if ( cp2 != cp1 )
	    {
	    strdecode( cp1, cp1 );
	    argp[argn++] = cp1;
	    }
	}

    argp[argn] = (char*) 0;
    return argp;
    }


/* Set up CGI environment variables. Be real careful here to avoid
 * letting malicious clients overrun a buffer.  We don't have
 * to worry about freeing stuff since we're a sub-process.
*/
static char**
make_envp( int is_ssl, unsigned short conn_port )
    {
    static char* envp[50];
    int envn;
    char* cp;
    char buf[256];

    envn = 0;
    envp[envn++] = build_env( "PATH=%s", CGI_PATH );
    envp[envn++] = build_env( "LD_LIBRARY_PATH=%s", CGI_LD_LIBRARY_PATH );
    envp[envn++] = build_env( "SERVER_SOFTWARE=%s", SERVER_SOFTWARE );
    if ( ! vhost )
	cp = hostname;
    else
	cp = req_hostname;	/* already computed by virtual_file() */
    if ( cp != (char*) 0 )
	envp[envn++] = build_env( "SERVER_NAME=%s", cp );
    envp[envn++] = "GATEWAY_INTERFACE=CGI/1.1";
    envp[envn++] = "SERVER_PROTOCOL=HTTP/1.0";
    (void) snprintf( buf, sizeof(buf), "%d", (int) conn_port );
    envp[envn++] = build_env( "SERVER_PORT=%s", buf );
    envp[envn++] = build_env(
	"REQUEST_METHOD=%s", get_method_str( method ) );
    envp[envn++] = build_env( "SCRIPT_NAME=%s", path );
    if ( pathinfo != (char*) 0 )
	{
	envp[envn++] = build_env( "PATH_INFO=/%s", pathinfo );
	(void) snprintf( buf, sizeof(buf), "%s%s", cwd, pathinfo );
	envp[envn++] = build_env( "PATH_TRANSLATED=%s", buf );
	}
    if ( query[0] != '\0' )
	envp[envn++] = build_env( "QUERY_STRING=%s", query );
    envp[envn++] = build_env( "REMOTE_ADDR=%s", ntoa( &client_addr ) );
    if ( referer[0] != '\0' )
	envp[envn++] = build_env( "HTTP_REFERER=%s", referer );
    if ( useragent[0] != '\0' )
	envp[envn++] = build_env( "HTTP_USER_AGENT=%s", useragent );
    if ( cookie != (char*) 0 )
	envp[envn++] = build_env( "HTTP_COOKIE=%s", cookie );
    if ( host != (char*) 0 )
	envp[envn++] = build_env( "HTTP_HOST=%s", host );
    if ( content_type != (char*) 0 )
	envp[envn++] = build_env( "CONTENT_TYPE=%s", content_type );
    if ( content_length != -1 )
	{
	(void) snprintf(
	    buf, sizeof(buf), "%lu", (unsigned long) content_length );
	envp[envn++] = build_env( "CONTENT_LENGTH=%s", buf );
	}
    if ( remoteuser != (char*) 0 )
	envp[envn++] = build_env( "REMOTE_USER=%s", remoteuser );
    if ( authorization != (char*) 0 )
	envp[envn++] = build_env( "AUTH_TYPE=%s", "Basic" );
    if ( getenv( "TZ" ) != (char*) 0 )
	envp[envn++] = build_env( "TZ=%s", getenv( "TZ" ) );

    envp[envn] = (char*) 0;
    return envp;
    }


static char*
build_env( char* fmt, char* arg )
    {
    char* cp;
    int size;
    static char* buf;
    static int maxbuf = 0;

    size = strlen( fmt ) + strlen( arg );
    if ( size > maxbuf )
	{
	if ( maxbuf == 0 )
	    {
	    maxbuf = MAX( 200, size + 100 );
	    buf = (char*) e_malloc( maxbuf );
	    }
	else
	    {
	    maxbuf = MAX( maxbuf * 2, size * 5 / 4 );
	    buf = (char*) e_realloc( (void*) buf, maxbuf );
	    }
	}
    (void) snprintf( buf, maxbuf, fmt, arg );
    cp = e_strdup( buf );
    return cp;
    }


/* this whole auth_check function has been heavily modified for the gargoyle additions */
static void auth_check( char* dirname, int is_ssl )
{
	char authpath[10000];
	char realmName[500];
	struct stat sb;
	char authinfo[500];
	char* authpass;
	char* colon;
	static char line[10000];
	int l;
	FILE* fp;
	char* cryp;

	
	/* Construct auth filename. */
	if ( dirname[strlen(dirname) - 1] == '/' )
	{
		(void) snprintf( authpath, sizeof(authpath), "%s%s", dirname, AUTH_FILE );
	}
	else
	{
		(void) snprintf( authpath, sizeof(authpath), "%s/%s", dirname, AUTH_FILE );
	}

	/* Does this directory have an auth file? */
	if ( stat( authpath, &sb ) < 0 )
	{
		/* is there a default realm ? */
		if(defaultRealmName == NULL || defaultRealmPasswordFile == NULL)
		{
			/* Nope, let the request go through. */
			return;
		}
		else
		{
			/* Use default realm info */
			snprintf( authpath, sizeof(authpath), "%s", defaultRealmPasswordFile);
		}
	}

	/* If auth file is default realm password file, realm should be the default realm,
	 *  otherwise it should be the directory name */
	if(strcmp(authpath, defaultRealmPasswordFile) == 0)
	{
		snprintf( realmName, sizeof(realmName), "%s", defaultRealmName);
	}
	else
	{
		snprintf( realmName, sizeof(realmName), "%s", dirname);
	}


	/* Does this request contain authorization info? */
	if ( authorization == (char*) 0 )
	{
		/* Nope, return a 401 Unauthorized. */
		send_authenticate( realmName, is_ssl );
	}

	/* Basic authorization info? */
	if ( strncmp( authorization, "Basic ", 6 ) != 0 )
	{
		send_authenticate( realmName, is_ssl );
	}

	/* Decode it. */
	l = b64_decode(
	&(authorization[6]), (unsigned char*) authinfo, sizeof(authinfo) - 1 );
	authinfo[l] = '\0';
	/* Split into user and password. */
	authpass = strchr( authinfo, ':' );
	if ( authpass == (char*) 0 )
	{
		/* No colon?  Bogus auth info. */
		send_authenticate( realmName, is_ssl );
	}
	*authpass++ = '\0';
	/* If there are more f:ields, cut them off. */
	colon = strchr( authpass, ':' );
	if ( colon != (char*) 0 )
	{
		*colon = '\0';
	}

	/* Open the password file. */
	fp = fopen( authpath, "r" );
	if ( fp == (FILE*) 0 )
	{
		/* The file exists but we can't open it?  Disallow access. */
		syslog(LOG_ERR, "%.80s auth file %.80s could not be opened - %m", ntoa( &client_addr ), authpath );
		send_error( 403, "Forbidden", "", "File is protected.", is_ssl );
	}

	/* Read it. */
	while ( fgets( line, sizeof(line), fp ) != (char*) 0 )
	{
		/* Nuke newline. */
		l = strlen( line );
		if ( line[l - 1] == '\n' )
		{
			line[l - 1] = '\0';
		}
	
		/* Split into user and encrypted password. */
		cryp = strchr( line, ':' );
		if ( cryp == (char*) 0 )
		{
			continue;
		}
		*cryp++ = '\0';
		
		/* Is this the right user? */
		if ( strcmp( line, authinfo ) == 0 )
		{
			/* Yes. */
			(void) fclose( fp );
			
			/* So is the password right? */
			if ( strcmp( crypt( authpass, cryp ), cryp ) == 0 )
			{
				/* Ok! */
				remoteuser = line;
				return;
			}
			else
			{
				/* No. */
				send_authenticate( realmName, is_ssl );
			}
		}
	}

	/* Didn't find that user.  Access denied. */
	(void) fclose( fp );
	send_authenticate( realmName, is_ssl );
}
/* end of function modified by gargoyle */



static void send_authenticate( char* realm, int is_ssl )
{
	char header[10000];
	(void) snprintf(header, sizeof(header), "WWW-Authenticate: Basic realm=\"%s\"", realm );
	send_error( 401, "Unauthorized", header, "Authorization required.", is_ssl );
}


static char* virtual_file( char* file )
{
	char* cp;
	static char vfile[10000];

	/* Use the request's hostname, or fall back on the IP address. */
	if ( host != (char*) 0 )
	{
		req_hostname = host;
	}
	else
	{
		usockaddr usa;
		int sz = sizeof(usa);
		if ( getsockname( conn_fd, &usa.sa, &sz ) < 0 )
		{
			req_hostname = "UNKNOWN_HOST";
		}
		else
		{
			req_hostname = ntoa( &usa );
		}
	}
	/* Pound it to lower case. */
	for ( cp = req_hostname; *cp != '\0'; ++cp )
	{
		if ( isupper( *cp ) )
		{
			*cp = tolower( *cp );
		}
	}
	(void) snprintf( vfile, sizeof(vfile), "%s/%s", req_hostname, file );
	return vfile;
}

static void send_redirect(char* extra_header, char* hostname, char* new_location, int is_ssl)
{
	char extra_header_buf[5000];
	const char *sep = new_location[0] == '/' ? "" : "/";
	const char *proto = is_ssl == 1 ? "https://" : "http://";
	extra_header = extra_header == NULL ? "" : extra_header;
	if(strcmp(extra_header, "") == 0)
	{
		sprintf(extra_header_buf, "Location: %s%s%s%s", proto, hostname, sep, new_location);
	}
	else
	{
		sprintf(extra_header_buf, "%s\r\nLocation: %s%s%s%s", extra_header, proto, hostname, sep, new_location);
	}
	add_headers(301, "Moved Permanently", extra_header_buf, "", default_content_type, (off_t) -1, (time_t) -1 );
	send_error_body(301, "Moved Permanently", "Moved Permanently" );
	send_error_tail();
	send_response(is_ssl);

#ifdef HAVE_SSL
	SSL_free( ssl );
#endif /* HAVE_SSL */
	exit( 1 );

}

static void send_error( int s, char* title, char* extra_header, char* text, int is_ssl )
{
	add_headers(s, title, extra_header, "", default_content_type, (off_t) -1, (time_t) -1 );
	send_error_body( s, title, text );
	send_error_tail();
	send_response(is_ssl);

#ifdef HAVE_SSL
	SSL_free( ssl );
#endif /* HAVE_SSL */
	exit( 1 );
}


static void send_error_body( int s, char* title, char* text)
{
	char filename[1000];
	char buf[10000];
	int buflen;

	if ( vhost && req_hostname != (char*) 0 )
	{
		/* Try virtual-host custom error page. */
		(void) snprintf(filename, sizeof(filename), "%s/%s/err%d.html",req_hostname, ERR_DIR, s );
		if ( send_error_file( filename ) )
		{
			return;
		}
	}

	/* Try server-wide custom error page. */
	(void) snprintf( filename, sizeof(filename), "%s/err%d.html", ERR_DIR, s );
	if ( send_error_file( filename ) )
	{
		return;
	}

	/* Send built-in error page. */
	buflen = snprintf(buf, sizeof(buf), "<HTML>\n<HEAD><TITLE>%d %s</TITLE></HEAD>\n<BODY BGCOLOR=\"#cc9999\" TEXT=\"#000000\" LINK=\"#2020ff\" VLINK=\"#4040cc\">\n<H4>%d %s</H4>\n",	s, title, s, title );
	add_to_response( buf, buflen );
	buflen = snprintf( buf, sizeof(buf), "%s\n", text );
	add_to_response( buf, buflen );
}


static int
send_error_file( char* filename )
    {
    FILE* fp;
    char buf[1000];
    size_t r;

    fp = fopen( filename, "r" );
    if ( fp == (FILE*) 0 )
	return 0;
    for (;;)
	{
	r = fread( buf, 1, sizeof(buf), fp );
	if ( r == 0 )
	    break;
	add_to_response( buf, r );
	}
    (void) fclose( fp );
    return 1;
    }


static void
send_error_tail( void )
    {
    char buf[500];
    int buflen;

    if ( match( "**MSIE**", useragent ) )
	{
	int n;
	buflen = snprintf( buf, sizeof(buf), "<!--\n" );
	add_to_response( buf, buflen );
	for ( n = 0; n < 6; ++n )
	    {
	    buflen = snprintf( buf, sizeof(buf), "Padding so that MSIE deigns to show this error instead of its own canned one.\n" );
	    add_to_response( buf, buflen );
	    }
	buflen = snprintf( buf, sizeof(buf), "-->\n" );
	add_to_response( buf, buflen );
	}

    buflen = snprintf( buf, sizeof(buf), "\
<HR>\n\
<ADDRESS><A HREF=\"%s\">%s</A></ADDRESS>\n\
</BODY>\n\
</HTML>\n",
	SERVER_URL, SERVER_SOFTWARE );
    add_to_response( buf, buflen );
    }


static void add_headers( int s, char* title, char* extra_header, char* me, char* mt, off_t b, time_t mod )
{
	time_t now;
	char timebuf[100];
	char buf[10000];
	int buflen;
	const char* rfc1123_fmt = "%a, %d %b %Y %H:%M:%S GMT";

	status = s;
	bytes = b;
	make_log_entry();
	start_response();
	buflen = snprintf( buf, sizeof(buf), "%s %d %s\015\012", protocol, status, title );
	add_to_response( buf, buflen );
	buflen = snprintf( buf, sizeof(buf), "Server: %s\015\012", SERVER_SOFTWARE );
	add_to_response( buf, buflen );
	now = time( (time_t*) 0 );
	(void) strftime( timebuf, sizeof(timebuf), rfc1123_fmt, gmtime( &now ) );
	buflen = snprintf( buf, sizeof(buf), "Date: %s\015\012", timebuf );
	add_to_response( buf, buflen );
	
	/* never cache */
	buflen = snprintf( buf, sizeof(buf), "Expires: %s\015\012", timebuf );
	add_to_response( buf, buflen );
	

	if ( mod != (time_t) -1 )
	{
		(void) strftime(timebuf, sizeof(timebuf), rfc1123_fmt, gmtime( &mod ) );
		buflen = snprintf( buf, sizeof(buf), "Last-Modified: %s\015\012", timebuf );
		add_to_response( buf, buflen );
	}
	if ( extra_header != (char*) 0 && extra_header[0] != '\0' )
	{
		buflen = snprintf( buf, sizeof(buf), "%s\015\012", extra_header );
		add_to_response( buf, buflen );
	}
	if ( me != (char*) 0 && me[0] != '\0' )
	{
		buflen = snprintf( buf, sizeof(buf), "Content-Encoding: %s\015\012", me );
		add_to_response( buf, buflen );
	}
	if ( mt != (char*) 0 && mt[0] != '\0' )
	{
		buflen = snprintf( buf, sizeof(buf), "Content-Type: %s\015\012", mt );
		add_to_response( buf, buflen );
	}
	if ( bytes >= 0 )
	{
		buflen = snprintf(buf, sizeof(buf), "Content-Length: %lld\015\012", (long long int) bytes );
		add_to_response( buf, buflen );
	}
	if ( p3p != (char*) 0 && p3p[0] != '\0' )
	{
		buflen = snprintf( buf, sizeof(buf), "P3P: %s\015\012", p3p );
		add_to_response( buf, buflen );
	}

	/* end of headers */
	buflen = snprintf( buf, sizeof(buf), "\015\012" );
	add_to_response( buf, buflen );
}


static void
start_request( void )
    {
    request_size = 0;
    request_idx = 0;
    }

static void
add_to_request( char* str, size_t len )
    {
    add_to_buf( &request, &request_size, &request_len, str, len );
    }

static char*
get_request_line( void )
    {
    int i;
    char c;

    for ( i = request_idx; request_idx < request_len; ++request_idx )
	{
	c = request[request_idx];
	if ( c == '\012' || c == '\015' )
	    {
	    request[request_idx] = '\0';
	    ++request_idx;
	    if ( c == '\015' && request_idx < request_len &&
		 request[request_idx] == '\012' )
		{
		request[request_idx] = '\0';
		++request_idx;
		}
	    return &(request[i]);
	    }
	}
    return (char*) 0;
    }


static char* response;
static size_t response_size, response_len;

static void
start_response( void )
    {
    response_size = 0;
    }

static void
add_to_response( char* str, size_t len )
    {
    add_to_buf( &response, &response_size, &response_len, str, len );
    }

static void
send_response( is_ssl )
    {
    (void) my_write( response, response_len, is_ssl );
    }


static void
send_via_write( int fd, off_t size, int is_ssl )
    {
    if ( size <= SIZE_T_MAX )
	{
	size_t size_size = (size_t) size;
	void* ptr = mmap( 0, size_size, PROT_READ, MAP_PRIVATE, fd, 0 );
	if ( ptr != (void*) -1 )
	    {
	    (void) my_write( ptr, size_size, is_ssl );
	    (void) munmap( ptr, size_size );
	    }
#ifdef MADV_SEQUENTIAL
	/* If we have madvise, might as well call it.  Although sequential
	** access is probably already the default.
	*/
	(void) madvise( ptr, size_size, MADV_SEQUENTIAL );
#endif /* MADV_SEQUENTIAL */
	}
    else
	{
	/* mmap can't deal with files larger than 2GB. */
	char buf[30000];
	ssize_t r, r2;

	for (;;)
	    {
	    r = read( fd, buf, sizeof(buf) );
	    if ( r < 0 && ( errno == EINTR || errno == EAGAIN ) )
		{
		sleep( 1 );
		continue;
		}
	    if ( r <= 0 )
		return;
	    for (;;)
		{
		r2 = my_write( buf, r, is_ssl );
		if ( r2 < 0 && ( errno == EINTR || errno == EAGAIN ) )
		    {
		    sleep( 1 );
		    continue;
		    }
		if ( r2 != r )
		    return;
		break;
		}
	    }
	}
    }


static ssize_t
my_read( char* buf, size_t size, int is_ssl )
    {
#ifdef HAVE_SSL
    if ( is_ssl )
	return SSL_read( ssl, buf, size );
    else
	return read( conn_fd, buf, size );
#else /* HAVE_SSL */
    return read( conn_fd, buf, size );
#endif /* HAVE_SSL */
    }


static ssize_t
my_write( char* buf, size_t size, int is_ssl )
    {
#ifdef HAVE_SSL
    if ( is_ssl )
	return SSL_write( ssl, buf, size );
    else
	return write( conn_fd, buf, size );
#else /* HAVE_SSL */
    return write( conn_fd, buf, size );
#endif /* HAVE_SSL */
    }


#ifdef HAVE_SENDFILE
static int
my_sendfile( int fd, int socket, off_t offset, size_t nbytes )
    {
#ifdef HAVE_LINUX_SENDFILE
	off_t lo = offset;
	return sendfile( socket, fd, &lo, nbytes );
#else /* HAVE_LINUX_SENDFILE */
	return sendfile( fd, socket, offset, nbytes, (struct sf_hdtr*) 0, (off_t*) 0, 0 );
#endif /* HAVE_LINUX_SENDFILE */
    }
#endif /* HAVE_SENDFILE */


static void
add_to_buf( char** bufP, size_t* bufsizeP, size_t* buflenP, char* str, size_t len )
    {
    if ( *bufsizeP == 0 )
	{
	*bufsizeP = len + 500;
	*buflenP = 0;
	*bufP = (char*) e_malloc( *bufsizeP );
	}
    else if ( *buflenP + len >= *bufsizeP )
	{
	*bufsizeP = *buflenP + len + 500;
	*bufP = (char*) e_realloc( (void*) *bufP, *bufsizeP );
	}
    (void) memmove( &((*bufP)[*buflenP]), str, len );
    *buflenP += len;
    (*bufP)[*buflenP] = '\0';
    }


static void
make_log_entry( void )
    {
    char* ru;
    char url[500];
    char bytes_str[40];
    time_t now;
    struct tm* t;
    const char* cernfmt_nozone = "%d/%b/%Y:%H:%M:%S";
    char date_nozone[100];
    int zone;
    char sign;
    char date[100];

    if ( logfp == (FILE*) 0 )
	return;

    /* Fill in some null values. */
    if ( protocol == (char*) 0 )
	protocol = "UNKNOWN";
    if ( path == (char*) 0 )
	path = "";
    if ( req_hostname == (char*) 0 )
	req_hostname = hostname;

    /* Format the user. */
    if ( remoteuser != (char*) 0 )
	ru = remoteuser;
    else
	ru = "-";
    now = time( (time_t*) 0 );
    /* If we're vhosting, prepend the hostname to the url.  This is
    ** a little weird, perhaps writing separate log files for
    ** each vhost would make more sense.
    */
    if ( vhost )
	(void) snprintf( url, sizeof(url), "/%s%s", req_hostname, path );
    else
	(void) snprintf( url, sizeof(url), "%s", path );
    /* Format the bytes. */
    if ( bytes >= 0 )
	(void) snprintf(
	    bytes_str, sizeof(bytes_str), "%lld", (long long int) bytes );
    else
	(void) strcpy( bytes_str, "-" );
    /* Format the time, forcing a numeric timezone (some log analyzers
    ** are stoooopid about this).
    */
    t = localtime( &now );
    (void) strftime( date_nozone, sizeof(date_nozone), cernfmt_nozone, t );
#ifdef HAVE_TM_GMTOFF
    zone = t->tm_gmtoff / 60L;
#else
    zone = - ( timezone / 60L );
    /* Probably have to add something about daylight time here. */
#endif
    if ( zone >= 0 )
	sign = '+';
    else
	{
	sign = '-';
	zone = -zone;
	}
    zone = ( zone / 60 ) * 100 + zone % 60;
    (void) snprintf( date, sizeof(date), "%s %c%04d", date_nozone, sign, zone );
    /* And write the log entry. */
    (void) fprintf( logfp,
	"%.80s - %.80s [%s] \"%.80s %.200s %.80s\" %d %s \"%.200s\" \"%.200s\"\n",
	ntoa( &client_addr ), ru, date, get_method_str( method ), url,
	protocol, status, bytes_str, referer, useragent );
    (void) fflush( logfp );
    }


/* Returns if it's ok to serve the url, otherwise generates an error
 * and exits.
*/
static void
check_referer( int is_ssl )
    {
    char* cp;

    /* Are we doing referer checking at all? */
    if ( url_pattern == (char*) 0 )
	return;

    /* Is it ok? */
    if ( really_check_referer() )
	return;

    /* Lose. */
    if ( vhost && req_hostname != (char*) 0 )
	cp = req_hostname;
    else
	cp = hostname;
    if ( cp == (char*) 0 )
	cp = "";
    syslog(
	LOG_INFO, "%.80s non-local referer \"%.80s%.80s\" \"%.80s\"",
	ntoa( &client_addr ), cp, path, referer );
    send_error( 403, "Forbidden", "", "You must supply a local referer.", is_ssl );
    }


/* Returns 1 if ok to serve the url, 0 if not. */
static int
really_check_referer( void )
    {
    char* cp1;
    char* cp2;
    char* cp3;
    char* refhost;
    char *lp;

    /* Check for an empty referer. */
    if ( referer == (char*) 0 || referer[0] == '\0' ||
	 ( cp1 = strstr( referer, "//" ) ) == (char*) 0 )
	{
	/* Disallow if we require a referer and the url matches. */
	if ( no_empty_referers && match( url_pattern, path ) )
	    return 0;
	/* Otherwise ok. */
	return 1;
	}

    /* Extract referer host. */
    cp1 += 2;
    for ( cp2 = cp1; *cp2 != '/' && *cp2 != ':' && *cp2 != '\0'; ++cp2 )
	continue;
    refhost = (char*) e_malloc( cp2 - cp1 + 1 );
    for ( cp3 = refhost; cp1 < cp2; ++cp1, ++cp3 )
	if ( isupper(*cp1) )
	    *cp3 = tolower(*cp1);
	else
	    *cp3 = *cp1;
    *cp3 = '\0';

    /* Local pattern? */
    if ( local_pattern != (char*) 0 )
	lp = local_pattern;
    else
	{
	/* No local pattern.  What's our hostname? */
	if ( ! vhost )
	    {
	    /* Not vhosting, use the server name. */
	    lp = hostname;
	    if ( lp == (char*) 0 )
		/* Couldn't figure out local hostname - give up. */
		return 1;
	    }
	else
	    {
	    /* We are vhosting, use the hostname on this connection. */
	    lp = req_hostname;
	    if ( lp == (char*) 0 )
		/* Oops, no hostname.  Maybe it's an old browser that
		** doesn't send a Host: header.  We could figure out
		** the default hostname for this IP address, but it's
		** not worth it for the few requests like this.
		*/
		return 1;
	    }
	}

    /* If the referer host doesn't match the local host pattern, and
    ** the URL does match the url pattern, it's an illegal reference.
    */
    if ( ! match( lp, refhost ) && match( url_pattern, path ) )
	return 0;
    /* Otherwise ok. */
    return 1;
    }


static char*
get_method_str( int m )
    {
    switch ( m )
	{
	case METHOD_GET: return "GET";
	case METHOD_HEAD: return "HEAD";
	case METHOD_POST: return "POST";
	default: return "UNKNOWN";
	}
    }


struct mime_entry {
    char* ext;
    size_t ext_len;
    char* val;
    size_t val_len;
    };
static struct mime_entry enc_tab[] = {
#include "mime_encodings.h"
    };
static const int n_enc_tab = sizeof(enc_tab) / sizeof(*enc_tab);
static struct mime_entry typ_tab[] = {
#include "mime_types.h"
    };
static const int n_typ_tab = sizeof(typ_tab) / sizeof(*typ_tab);


/* qsort comparison routine - declared old-style on purpose, for portability. */
static int
ext_compare( a, b )
    struct mime_entry* a;
    struct mime_entry* b;
    {
    return strcmp( a->ext, b->ext );
    }


static void
init_mime( void )
    {
    int i;

    /* Sort the tables so we can do binary search. */
    qsort( enc_tab, n_enc_tab, sizeof(*enc_tab), ext_compare );
    qsort( typ_tab, n_typ_tab, sizeof(*typ_tab), ext_compare );

    /* Fill in the lengths. */
    for ( i = 0; i < n_enc_tab; ++i )
	{
	enc_tab[i].ext_len = strlen( enc_tab[i].ext );
	enc_tab[i].val_len = strlen( enc_tab[i].val );
	}
    for ( i = 0; i < n_typ_tab; ++i )
	{
	typ_tab[i].ext_len = strlen( typ_tab[i].ext );
	typ_tab[i].val_len = strlen( typ_tab[i].val );
	}
    }


/* Figure out MIME encodings and type based on the filename.  Multiple
 * encodings are separated by commas, and are listed in the order in
 * which they were applied to the file.
*/
static const char*
figure_mime( char* name, char* me, size_t me_size )
    {
    char* prev_dot;
    char* dot;
    char* ext;
    int me_indexes[100], n_me_indexes;
    size_t ext_len, me_len;
    int i, top, bot, mid;
    int r;
    const char* default_type = "application/octet-stream ";
    const char* type;

    /* Peel off encoding extensions until there aren't any more. */
    n_me_indexes = 0;
    for ( prev_dot = &name[strlen(name)]; ; prev_dot = dot )
	{
	for ( dot = prev_dot - 1; dot >= name && *dot != '.'; --dot )
	    ;
	if ( dot < name )
	    {
	    /* No dot found.  No more encoding extensions, and no type
	    ** extension either.
	    */
	    type = default_type;
	    goto done;
	    }
	ext = dot + 1;
	ext_len = prev_dot - ext;
	/* Search the encodings table.  Linear search is fine here, there
	** are only a few entries.
	*/
	for ( i = 0; i < n_enc_tab; ++i )
	    {
	    if ( ext_len == enc_tab[i].ext_len && strncasecmp( ext, enc_tab[i].ext, ext_len ) == 0 )
		{
		if ( n_me_indexes < sizeof(me_indexes)/sizeof(*me_indexes) )
		    {
		    me_indexes[n_me_indexes] = i;
		    ++n_me_indexes;
		    }
		goto next;
		}
	    }
	/* No encoding extension found.  Break and look for a type extension. */
	break;

	next: ;
	}

    /* Binary search for a matching type extension. */
    top = n_typ_tab - 1;
    bot = 0;
    while ( top >= bot )
	{
	mid = ( top + bot ) / 2;
	r = strncasecmp( ext, typ_tab[mid].ext, ext_len );
	if ( r < 0 )
	    top = mid - 1;
	else if ( r > 0 )
	    bot = mid + 1;
	else
	    if ( ext_len < typ_tab[mid].ext_len )
		top = mid - 1;
	    else if ( ext_len > typ_tab[mid].ext_len )
		bot = mid + 1;
	    else
		{
		type = typ_tab[mid].val;
		goto done;
		}
	}
    type = default_type;

    done:

    /* The last thing we do is actually generate the mime-encoding header. */
    me[0] = '\0';
    me_len = 0;
    for ( i = n_me_indexes - 1; i >= 0; --i )
	{
	if ( me_len + enc_tab[me_indexes[i]].val_len + 1 < me_size )
	    {
	    if ( me[0] != '\0' )
		{
		(void) strcpy( &me[me_len], "," );
		++me_len;
		}
	    (void) strcpy( &me[me_len], enc_tab[me_indexes[i]].val );
	    me_len += enc_tab[me_indexes[i]].val_len;
	    }
	}

    return type;
    }


static void
handle_sigterm( int sig )
    {
    /* Don't need to set up the handler again, since it's a one-shot. */

    syslog( LOG_NOTICE, "exiting due to signal %d", sig );
    (void) fprintf( stderr, "%s: exiting due to signal %d\n", argv0, sig );
    closelog();
    exit( 1 );
    }


/* SIGHUP says to re-open the log file. */
static void
handle_sighup( int sig )
    {
    const int oerrno = errno;

#ifndef HAVE_SIGSET
    /* Set up handler again. */
    (void) signal( SIGHUP, handle_sighup );
#endif /* ! HAVE_SIGSET */

    /* Just set a flag that we got the signal. */
    got_hup = 1;
	
    /* Restore previous errno. */
    errno = oerrno;
    }


static void
handle_sigchld( int sig )
    {
    const int oerrno = errno;
    pid_t pid;
    int status;

#ifndef HAVE_SIGSET
    /* Set up handler again. */
    (void) signal( SIGCHLD, handle_sigchld );
#endif /* ! HAVE_SIGSET */

    /* Reap defunct children until there aren't any more. */
    for (;;)
	{
#ifdef HAVE_WAITPID
	pid = waitpid( (pid_t) -1, &status, WNOHANG );
#else /* HAVE_WAITPID */
	pid = wait3( &status, WNOHANG, (struct rusage*) 0 );
#endif /* HAVE_WAITPID */
	if ( (int) pid == 0 )		/* none left */
	    break;
	if ( (int) pid < 0 )
	    {
	    if ( errno == EINTR || errno == EAGAIN )
		continue;
	    /* ECHILD shouldn't happen with the WNOHANG option,
	    ** but with some kernels it does anyway.  Ignore it.
	    */
	    if ( errno != ECHILD )
		{
		syslog( LOG_ERR, "child wait - %m" );
		perror( "child wait" );
		}
	    break;
	    }
	}

    /* Restore previous errno. */
    errno = oerrno;
    }


static void
re_open_logfile( void )
    {
    if ( logfp != (FILE*) 0 )
	{
	(void) fclose( logfp );
	logfp = (FILE*) 0;
	}
    if ( logfile != (char*) 0 )
	{
	syslog( LOG_NOTICE, "re-opening logfile" );
	logfp = fopen( logfile, "a" );
	if ( logfp == (FILE*) 0 )
	    {
	    syslog( LOG_CRIT, "%s - %m", logfile );
	    perror( logfile );
	    exit( 1 );
	    }
	}
    }

static void handle_read_timeout_sig(int sig)
{
	handle_read_timeout(0,0);
}

static void handle_read_timeout( int sig, int is_ssl )
{
    syslog( LOG_INFO, "%.80s connection timed out reading", ntoa( &client_addr ) );
    send_error(
	408, "Request Timeout", "",
	"No request appeared within a reasonable time period.", is_ssl );
}


static void
handle_write_timeout( int sig )
    {
    syslog( LOG_INFO, "%.80s connection timed out writing", ntoa( &client_addr ) );
    exit( 1 );
    }


/* this function is heavily modified in gargoyle version to allow for connection with ssl & non-ssl on different ports */
static void lookup_hostname(	usockaddr* usa4P, 
				usockaddr* usa4sP, 
				size_t sa4_len, 
				int* gotv4P,
			        int* gotv4sP,	
				usockaddr* usa6P, 
				usockaddr* usa6sP, 
				size_t sa6_len, 
				int* gotv6P,
				int* gotv6sP
				)
{
	int port_index;
	unsigned short port_list[4];
       	port_list[0] = port;
	port_list[1] = sslPort;
	port_list[2] = 0;

	*gotv6P  = 0;
	*gotv6sP = 0;
	*gotv4P  = 0;
	*gotv4sP = 0;


#ifdef USE_IPV6

	for(port_index=0; port_list[port_index] != 0; port_index++)
	{
		struct addrinfo hints;
		char portstr[10];
		int gaierr;
		struct addrinfo* ai;
		struct addrinfo* ai2;
		struct addrinfo* aiv6;
		struct addrinfo* aiv4;

		(void) memset( &hints, 0, sizeof(hints) );
		hints.ai_family = PF_UNSPEC;
		hints.ai_flags = AI_PASSIVE;
		hints.ai_socktype = SOCK_STREAM;
		(void) snprintf( portstr, sizeof(portstr), "%d", (int) port_list[port_index] );
		if ( (gaierr = getaddrinfo( hostname, portstr, &hints, &ai )) != 0 )
		{
			syslog(	LOG_CRIT, 
				"getaddrinfo %.80s - %s", 
				hostname,
				gai_strerror( gaierr ) 
				);
			(void) fprintf(	stderr, 
					"%s: getaddrinfo %.80s - %s\n", 
					argv0, 
					hostname,
					gai_strerror( gaierr ) 
					);
			exit( 1 );
		}

		/* Find the first IPv6 and IPv4 entries. */
		aiv6 = (struct addrinfo*) 0;
		aiv4 = (struct addrinfo*) 0;
		for ( ai2 = ai; ai2 != (struct addrinfo*) 0; ai2 = ai2->ai_next )
		{
			switch ( ai2->ai_family )
			{
	    			case AF_INET6:
	    				if ( aiv6 == (struct addrinfo*) 0 ) { aiv6 = ai2; }
	    				break;
	    			case AF_INET:
	    				if ( aiv4 == (struct addrinfo*) 0 ) { aiv4 = ai2; }
					break;
			}
		}

		if ( aiv6 != (struct addrinfo*) 0 )
		{
			if ( sa6_len < aiv6->ai_addrlen )
			{
				syslog(	LOG_CRIT, 
					"%.80s - sockaddr too small (%lu < %lu)",
					hostname, 
					(unsigned long) sa6_len,
					(unsigned long) aiv6->ai_addrlen 
					);
				(void) fprintf(	stderr, 
						"%s: %.80s - sockaddr too small (%lu < %lu)\n",
						argv0, 
						hostname, 
						(unsigned long) sa6_len,
						(unsigned long) aiv6->ai_addrlen 
						);
				exit( 1 );
			}
			if(port_index==0)
			{
				*gotv6P = 1;
				(void) memset( usa6P, 0, sa6_len );
				(void) memmove( usa6P, aiv6->ai_addr, aiv6->ai_addrlen );
			}
			else
			{
				*gotv6sP = 1;
				(void) memset( usa6sP, 0, sa6_len );
				(void) memmove( usa6sP, aiv6->ai_addr, aiv6->ai_addrlen );

			}
		}

		if ( aiv4 != (struct addrinfo*) 0 )
		{
			if ( sa4_len < aiv4->ai_addrlen )
			{
				syslog(	LOG_CRIT, 
					"%.80s - sockaddr too small (%lu < %lu)",
					hostname, 
					(unsigned long) sa4_len,
					(unsigned long) aiv4->ai_addrlen 
					);
				(void) fprintf(	stderr, 
						"%s: %.80s - sockaddr too small (%lu < %lu)\n",
						argv0, 
						hostname, 
						(unsigned long) sa4_len,
						(unsigned long) aiv4->ai_addrlen 
						);
				exit( 1 );
			}
			if(port_index==0)
			{
				*gotv4P = 1;
				(void) memset( usa4P, 0, sa4_len );
				(void) memmove( usa4P, aiv4->ai_addr, aiv4->ai_addrlen );
			}
			else
			{
				*gotv4sP = 1;
				(void) memset( usa4sP, 0, sa4_len );
				(void) memmove( usa4sP, aiv4->ai_addr, aiv4->ai_addrlen );

			}
			*gotv4P = 1;
		}

    		freeaddrinfo( ai );
	}

#else /* USE_IPV6 */

	
	for(port_index=0; port_list[port_index] != 0; port_index++)
	{
		usockaddr* nextUsa = port_index == 0 ? usa4P : usa4sP;

		(void) memset( nextUsa, 0, sa4_len );
		nextUsa->sa.sa_family = AF_INET;
		if ( hostname == (char*) 0 )
		{
			nextUsa->sa_in.sin_addr.s_addr = htonl( INADDR_ANY );
		}
		else
		{
			nextUsa->sa_in.sin_addr.s_addr = inet_addr( hostname );
			if ( (int) nextUsa->sa_in.sin_addr.s_addr == -1 )
			{
				struct hostent* he;
				he = gethostbyname( hostname );
				if ( he == (struct hostent*) 0 )
				{
#ifdef HAVE_HSTRERROR
					syslog(	LOG_CRIT, 
						"gethostbyname %.80s - %s", 
						hostname,
						hstrerror( h_errno ) 
						);
					(void) fprintf(	stderr, 
							"%s: gethostbyname %.80s - %s\n", 
							argv0, 
							hostname,
							hstrerror( h_errno ) 
							);
#else /* HAVE_HSTRERROR */
					syslog(	LOG_CRIT, "gethostbyname %.80s failed", hostname );
					(void) fprintf( stderr, "%s: gethostbyname %.80s failed\n", argv0, hostname );
#endif /* HAVE_HSTRERROR */
					exit( 1 );
				}
				if ( he->h_addrtype != AF_INET )
				{
					syslog( LOG_CRIT, "%.80s - non-IP network address", hostname );
					(void) fprintf( stderr, "%s: %.80s - non-IP network address\n", argv0, hostname );
					exit( 1 );
				}
				(void) memmove( &nextUsa->sa_in.sin_addr.s_addr, he->h_addr, he->h_length );
			}
		}
		nextUsa->sa_in.sin_port = htons( port_list[port] );
		if(port_index==0)
		{
			*gotv4P = 1;
		}
		else
		{
			*gotv4sP = 1;
		}
	}
#endif /* USE_IPV6 */
}
/* end modified gargoyle function */


static char*
ntoa( usockaddr* usaP )
    {
#ifdef USE_IPV6
    static char str[200];

    if ( getnameinfo( &usaP->sa, sockaddr_len( usaP ), str, sizeof(str), 0, 0, NI_NUMERICHOST ) != 0 )
	{
	str[0] = '?';
	str[1] = '\0';
	}
    else if ( IN6_IS_ADDR_V4MAPPED( &usaP->sa_in6.sin6_addr ) && strncmp( str, "::ffff:", 7 ) == 0 )
	/* Elide IPv6ish prefix for IPv4 addresses. */
	(void) strcpy( str, &str[7] );

    return str;

#else /* USE_IPV6 */

    return inet_ntoa( usaP->sa_in.sin_addr );

#endif /* USE_IPV6 */
    }


static int
sockaddr_check( usockaddr* usaP )
    {
    switch ( usaP->sa.sa_family )
	{
	case AF_INET: return 1;
#ifdef USE_IPV6
	case AF_INET6: return 1;
#endif /* USE_IPV6 */
	default:
	return 0;
	}
    }


static size_t
sockaddr_len( usockaddr* usaP )
    {
    switch ( usaP->sa.sa_family )
	{
	case AF_INET: return sizeof(struct sockaddr_in);
#ifdef USE_IPV6
	case AF_INET6: return sizeof(struct sockaddr_in6);
#endif /* USE_IPV6 */
	default:
	return 0;	/* shouldn't happen */
	}
    }


/* Copies and decodes a string.  It's ok for from and to to be the
 * same string.
*/
static void
strdecode( char* to, char* from )
    {
    for ( ; *from != '\0'; ++to, ++from )
	{
	if ( from[0] == '%' && isxdigit( from[1] ) && isxdigit( from[2] ) )
	    {
	    *to = hexit( from[1] ) * 16 + hexit( from[2] );
	    from += 2;
	    }
	else
	    *to = *from;
	}
    *to = '\0';
    }


static int
hexit( char c )
    {
    if ( c >= '0' && c <= '9' )
	return c - '0';
    if ( c >= 'a' && c <= 'f' )
	return c - 'a' + 10;
    if ( c >= 'A' && c <= 'F' )
	return c - 'A' + 10;
    return 0;           /* shouldn't happen, we're guarded by isxdigit() */
    }


/* Base-64 decoding.  This represents binary data as printable ASCII
 * characters.  Three 8-bit binary bytes are turned into four 6-bit
 * values, like so:
 *
 *   [11111111]  [22222222]  [33333333]
 *
 *   [111111] [112222] [222233] [333333]
 *
 * Then the 6-bit values are represented using the characters "A-Za-z0-9+/".
*/

static int b64_decode_table[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
    };

/* Do base-64 decoding on a string.  Ignore any non-base64 bytes.
 * Return the actual number of bytes generated.  The decoded size will
 * be at most 3/4 the size of the encoded, and may be smaller if there
 * are padding characters (blanks, newlines).
*/
static int
b64_decode( const char* str, unsigned char* space, int size )
    {
    const char* cp;
    int space_idx, phase;
    int d, prev_d = 0;
    unsigned char c;

    space_idx = 0;
    phase = 0;
    for ( cp = str; *cp != '\0'; ++cp )
	{
	d = b64_decode_table[(int) *cp];
	if ( d != -1 )
	    {
	    switch ( phase )
		{
		case 0:
		++phase;
		break;
		case 1:
		c = ( ( prev_d << 2 ) | ( ( d & 0x30 ) >> 4 ) );
		if ( space_idx < size )
		    space[space_idx++] = c;
		++phase;
		break;
		case 2:
		c = ( ( ( prev_d & 0xf ) << 4 ) | ( ( d & 0x3c ) >> 2 ) );
		if ( space_idx < size )
		    space[space_idx++] = c;
		++phase;
		break;
		case 3:
		c = ( ( ( prev_d & 0x03 ) << 6 ) | d );
		if ( space_idx < size )
		    space[space_idx++] = c;
		phase = 0;
		break;
		}
	    prev_d = d;
	    }
	}
    return space_idx;
    }


/* Set NDELAY mode on a socket. */
static void
set_ndelay( int fd )
    {
    int flags, newflags;

    flags = fcntl( fd, F_GETFL, 0 );
    if ( flags != -1 )
	{
	newflags = flags | (int) O_NDELAY;
	if ( newflags != flags )
	    (void) fcntl( fd, F_SETFL, newflags );
	}
    }


/* Clear NDELAY mode on a socket. */
static void
clear_ndelay( int fd )
    {
    int flags, newflags;

    flags = fcntl( fd, F_GETFL, 0 );
    if ( flags != -1 )
	{
	newflags = flags & ~ (int) O_NDELAY;
	if ( newflags != flags )
	    (void) fcntl( fd, F_SETFL, newflags );
	}
    }


static void*
e_malloc( size_t size )
    {
    void* ptr;

    ptr = malloc( size );
    if ( ptr == (void*) 0 )
	{
	syslog( LOG_CRIT, "out of memory" );
	(void) fprintf( stderr, "%s: out of memory\n", argv0 );
	exit( 1 );
	}
    return ptr;
    }


static void*
e_realloc( void* optr, size_t size )
    {
    void* ptr;

    ptr = realloc( optr, size );
    if ( ptr == (void*) 0 )
	{
	syslog( LOG_CRIT, "out of memory" );
	(void) fprintf( stderr, "%s: out of memory\n", argv0 );
	exit( 1 );
	}
    return ptr;
    }


static char*
e_strdup( char* ostr )
    {
    char* str;

    str = strdup( ostr );
    if ( str == (char*) 0 )
	{
	syslog( LOG_CRIT, "out of memory copying a string" );
	(void) fprintf( stderr, "%s: out of memory copying a string\n", argv0 );
	exit( 1 );
	}
    return str;
    }


#ifdef NO_SNPRINTF
/* Some systems don't have snprintf(), so we make our own that uses
 * vsprintf().  This workaround is probably vulnerable to buffer overruns,
 * so upgrade your OS!
*/
static int
snprintf( char* str, size_t size, const char* format, ... )
    {
    va_list ap;
    int r;

    va_start( ap, format );
    r = vsprintf( str, format, ap );
    va_end( ap );
    return r;
    }
#endif /* NO_SNPRINTF */
