/* vi: set sw=4 ts=4: */
/*
 * Busybox main internal header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef	__BBTARGZ_H__
#define	__BBTARGZ_H__    1

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>


#ifndef FALSE
#define FALSE   ((int) 0)
#endif

#ifndef TRUE
#define TRUE    ((int) 1)
#endif


#define targz_msg(l, fmt, args...) \
	do { \
		if (l == NOTICE) \
			targz_message(l, fmt, ##args); \
		else \
			targz_message(l, "%s: "fmt, __FUNCTION__, ##args); \
	} while (0)

#define targz_perror(l, fmt, args...) \
	targz_msg(l, fmt": %s.\n", ##args, strerror(errno))

#define error_msg(fmt, args...) targz_msg(ERROR, fmt"\n", ##args)
#define perror_msg(fmt, args...) targz_perror(ERROR, fmt, ##args)
#define error_msg_and_die(fmt, args...) \
	do { \
		error_msg(fmt, ##args); \
		exit(EXIT_FAILURE); \
	} while (0)
#define perror_msg_and_die(fmt, args...) \
	do { \
		perror_msg(fmt, ##args); \
		exit(EXIT_FAILURE); \
	} while (0)


typedef enum {
	ERROR,	/* error conditions */
	NOTICE,	/* normal but significant condition */
	INFO,	/* informational message */
	DEBUG,	/* debug level message */
	DEBUG2,	/* more debug level message */
} message_level_t;

void free_error_list(void);
void print_error_list(void);
void targz_message(message_level_t level, const char *fmt, ...)
				__attribute__ ((format (printf, 2, 3)));


extern void archive_xread_all(int fd, char *buf, size_t count);

const char *mode_string(int mode);
const char *time_string(time_t timeVal);

int copy_file(const char *source, const char *dest, int flags);
int copy_file_chunk(FILE *src_file, FILE *dst_file, unsigned long long chunksize);
ssize_t safe_read(int fd, void *buf, size_t count);
ssize_t full_read(int fd, char *buf, int len);

extern int parse_mode( const char* s, mode_t* theMode);

extern FILE *wfopen(const char *path, const char *mode);
extern FILE *xfopen(const char *path, const char *mode);

extern void *xmalloc (size_t size);
extern void *xrealloc(void *old, size_t size);
extern void *xcalloc(size_t nmemb, size_t size);
extern char *xstrdup (const char *s);
extern char *xstrndup (const char *s, int n);
extern char *safe_strncpy(char *dst, const char *src, size_t size);

char *xreadlink(const char *path);
char *concat_path_file(const char *path, const char *filename);
char *last_char_is(const char *s, int c);

typedef struct file_headers_s {
	char *name;
	char *link_name;
	off_t size;
	uid_t uid;
	gid_t gid;
	mode_t mode;
	time_t mtime;
	dev_t device;
} file_header_t;

enum extract_functions_e {
	extract_verbose_list = 1,
	extract_list = 2,
	extract_one_to_buffer = 4,
	extract_to_stream = 8,
	extract_all_to_fs = 16,
	extract_preserve_date = 32,
	extract_data_tar_gz = 64,
	extract_control_tar_gz = 128,
	extract_unzip_only = 256,
	extract_unconditional = 512,
	extract_create_leading_dirs = 1024,
	extract_quiet = 2048,
	extract_exclude_list = 4096
};

char *deb_extract(const char *package_filename, FILE *out_stream,
		const int extract_function, const char *prefix,
		const char *filename, int *err);

extern int unzip(FILE *l_in_file, FILE *l_out_file);
extern int gz_close(int gunzip_pid);
extern FILE *gz_open(FILE *compressed_file, int *pid);

int make_directory (const char *path, long mode, int flags);

enum {
	FILEUTILS_PRESERVE_STATUS = 1,
	FILEUTILS_PRESERVE_SYMLINKS = 2,
	FILEUTILS_RECUR = 4,
	FILEUTILS_FORCE = 8,
};




/* Declaration of functions and data types used for MD5 sum computing
   library functions.
   Copyright (C) 1995-1997,1999,2000,2001,2004,2005,2006,2008
      Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */



#define MD5_DIGEST_SIZE 16
#define MD5_BLOCK_SIZE 64

#ifndef __GNUC_PREREQ
# if defined __GNUC__ && defined __GNUC_MINOR__
#  define __GNUC_PREREQ(maj, min)					\
  ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
# else
#  define __GNUC_PREREQ(maj, min) 0
# endif
#endif

#ifndef __THROW
# if defined __cplusplus && __GNUC_PREREQ (2,8)
#  define __THROW	throw ()
# else
#  define __THROW
# endif
#endif

#ifndef _LIBC
# define __md5_buffer md5_buffer
# define __md5_finish_ctx md5_finish_ctx
# define __md5_init_ctx md5_init_ctx
# define __md5_process_block md5_process_block
# define __md5_process_bytes md5_process_bytes
# define __md5_read_ctx md5_read_ctx
# define __md5_stream md5_stream
#endif

/* Structure to save state of computation between the single steps.  */
struct md5_ctx
{
  uint32_t A;
  uint32_t B;
  uint32_t C;
  uint32_t D;

  uint32_t total[2];
  uint32_t buflen;
  uint32_t buffer[32];
};

/*
 * The following three functions are build up the low level used in
 * the functions `md5_stream' and `md5_buffer'.
 */

/* Initialize structure containing state of computation.
   (RFC 1321, 3.3: Step 3)  */
extern void __md5_init_ctx (struct md5_ctx *ctx) __THROW;

/* Starting with the result of former calls of this function (or the
   initialization function update the context for the next LEN bytes
   starting at BUFFER.
   It is necessary that LEN is a multiple of 64!!! */
extern void __md5_process_block (const void *buffer, size_t len,
				 struct md5_ctx *ctx) __THROW;

/* Starting with the result of former calls of this function (or the
   initialization function update the context for the next LEN bytes
   starting at BUFFER.
   It is NOT required that LEN is a multiple of 64.  */
extern void __md5_process_bytes (const void *buffer, size_t len,
				 struct md5_ctx *ctx) __THROW;

/* Process the remaining bytes in the buffer and put result from CTX
   in first 16 bytes following RESBUF.  The result is always in little
   endian byte order, so that a byte-wise output yields to the wanted
   ASCII representation of the message digest.  */
extern void *__md5_finish_ctx (struct md5_ctx *ctx, void *resbuf) __THROW;


/* Put result from CTX in first 16 bytes following RESBUF.  The result is
   always in little endian byte order, so that a byte-wise output yields
   to the wanted ASCII representation of the message digest.  */
extern void *__md5_read_ctx (const struct md5_ctx *ctx, void *resbuf) __THROW;


/* Compute MD5 message digest for bytes read from STREAM.  The
   resulting message digest number will be written into the 16 bytes
   beginning at RESBLOCK.  */
extern int __md5_stream (FILE *stream, void *resblock) __THROW;

/* Compute MD5 message digest for LEN bytes beginning at BUFFER.  The
   result is always in little endian byte order, so that a byte-wise
   output yields to the wanted ASCII representation of the message
   digest.  */
extern void *__md5_buffer (const char *buffer, size_t len,
			   void *resblock) __THROW;



char *file_md5sum_alloc(const char *file_name);




#endif /* __BBTARGZ_H__ */
