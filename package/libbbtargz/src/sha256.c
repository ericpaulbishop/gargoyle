/* sha256.c - Functions to compute SHA256 and SHA224 message digest of files or
   memory blocks according to the NIST specification FIPS-180-2.

   Copyright (C) 2005, 2006 Free Software Foundation, Inc.

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

/* Written by David Madore, considerably copypasting from
   Scott G. Miller's sha1.c
*/


#include "bbtargz.h"

#include <stddef.h>
#include <string.h>

#if USE_UNLOCKED_IO
#include "unlocked-io.h"
#endif



/* Endian detection yoinked from busybox */

#if defined(__digital__) && defined(__unix__)
# include <sex.h>
# define __BIG_ENDIAN__ (BYTE_ORDER == BIG_ENDIAN)
# define __BYTE_ORDER BYTE_ORDER
#elif defined __FreeBSD__
char *strchrnul(const char *s, int c);
# include <sys/resource.h>	/* rlimit */
# include <machine/endian.h>
# define bswap_64 __bswap64
# define bswap_32 __bswap32
# define bswap_16 __bswap16
# define __BIG_ENDIAN__ (_BYTE_ORDER == _BIG_ENDIAN)
#elif !defined __APPLE__
# include <byteswap.h>
# include <endian.h>
#endif

#if defined(__BIG_ENDIAN__) && __BIG_ENDIAN__
# define BB_BIG_ENDIAN 1
# define BB_LITTLE_ENDIAN 0
#elif __BYTE_ORDER == __BIG_ENDIAN
# define WORDS_BIGENDIAN 1
# define BB_BIG_ENDIAN 1
# define BB_LITTLE_ENDIAN 0
#elif __BYTE_ORDER == __LITTLE_ENDIAN
# define BB_BIG_ENDIAN 0
# define BB_LITTLE_ENDIAN 1
#else
# error "Can't determine endiannes"
#endif




#ifdef _LIBC
# if __BYTE_ORDER == __BIG_ENDIAN
#  define WORDS_BIGENDIAN 1
# endif
/* We need to keep the namespace clean so define the SHA256 function
   protected using leading __ .  */
# define sha256_init_ctx __sha256_init_ctx
# define sha256_process_block __sha256_process_block
# define sha256_process_bytes __sha256_process_bytes
# define sha256_finish_ctx __sha256_finish_ctx
# define sha256_read_ctx __sha256_read_ctx
# define sha256_stream __sha256_stream
# define sha256_buffer __sha256_buffer
#endif

#ifdef WORDS_BIGENDIAN
#define SWAP(n) (n)
#else
#define SWAP(n) \
    (((n) << 24) | (((n) & 0xff00) << 8) | (((n) >> 8) & 0xff00) | ((n) >> 24))
#endif

#define BLOCKSIZE 4096
#if BLOCKSIZE % 64 != 0
#error "invalid BLOCKSIZE"
#endif

/* This array contains the bytes used to pad the buffer to the next
   64-byte boundary.  */
static const unsigned char fillbuf[64] = { 0x80, 0 /* , 0, 0, ...  */  };

/*
  Takes a pointer to a 256 bit block of data (eight 32 bit ints) and
  intializes it to the start constants of the SHA256 algorithm.  This
  must be called before using hash in the call to sha256_hash
*/
void sha256_init_ctx(struct sha256_ctx *ctx)
{
	ctx->state[0] = 0x6a09e667UL;
	ctx->state[1] = 0xbb67ae85UL;
	ctx->state[2] = 0x3c6ef372UL;
	ctx->state[3] = 0xa54ff53aUL;
	ctx->state[4] = 0x510e527fUL;
	ctx->state[5] = 0x9b05688cUL;
	ctx->state[6] = 0x1f83d9abUL;
	ctx->state[7] = 0x5be0cd19UL;

	ctx->total[0] = ctx->total[1] = 0;
	ctx->buflen = 0;
}

/* Put result from CTX in first 32 bytes following RESBUF.  The result
   must be in little endian byte order.

   IMPORTANT: On some systems it is required that RESBUF is correctly
   aligned for a 32-bit value.  */
void *sha256_read_ctx(const struct sha256_ctx *ctx, void *resbuf)
{
	int i;

	for (i = 0; i < 8; i++)
		((uint32_t *) resbuf)[i] = SWAP(ctx->state[i]);

	return resbuf;
}

/* Process the remaining bytes in the internal buffer and the usual
   prolog according to the standard and write the result to RESBUF.

   IMPORTANT: On some systems it is required that RESBUF is correctly
   aligned for a 32-bit value.  */
static void sha256_conclude_ctx(struct sha256_ctx *ctx)
{
	/* Take yet unprocessed bytes into account.  */
	uint32_t bytes = ctx->buflen;
	size_t size = (bytes < 56) ? 64 / 4 : 64 * 2 / 4;

	/* Now count remaining bytes.  */
	ctx->total[0] += bytes;
	if (ctx->total[0] < bytes)
		++ctx->total[1];

	/* Put the 64-bit file length in *bits* at the end of the buffer.  */
	ctx->buffer[size - 2] =
	    SWAP((ctx->total[1] << 3) | (ctx->total[0] >> 29));
	ctx->buffer[size - 1] = SWAP(ctx->total[0] << 3);

	memcpy(&((char *)ctx->buffer)[bytes], fillbuf, (size - 2) * 4 - bytes);

	/* Process last bytes.  */
	sha256_process_block(ctx->buffer, size * 4, ctx);
}

void *sha256_finish_ctx(struct sha256_ctx *ctx, void *resbuf)
{
	sha256_conclude_ctx(ctx);
	return sha256_read_ctx(ctx, resbuf);
}

/* Compute SHA256 message digest for bytes read from STREAM.  The
   resulting message digest number will be written into the 32 bytes
   beginning at RESBLOCK.  */
int sha256_stream(FILE * stream, void *resblock)
{
	struct sha256_ctx ctx;
	char buffer[BLOCKSIZE + 72];
	size_t sum;

	/* Initialize the computation context.  */
	sha256_init_ctx(&ctx);

	/* Iterate over full file contents.  */
	while (1) {
		/* We read the file in blocks of BLOCKSIZE bytes.  One call of the
		   computation function processes the whole buffer so that with the
		   next round of the loop another block can be read.  */
		size_t n;
		sum = 0;

		/* Read block.  Take care for partial reads.  */
		while (1) {
			n = fread(buffer + sum, 1, BLOCKSIZE - sum, stream);

			sum += n;

			if (sum == BLOCKSIZE)
				break;

			if (n == 0) {
				/* Check for the error flag IFF N == 0, so that we don't
				   exit the loop after a partial read due to e.g., EAGAIN
				   or EWOULDBLOCK.  */
				if (ferror(stream))
					return 1;
				goto process_partial_block;
			}

			/* We've read at least one byte, so ignore errors.  But always
			   check for EOF, since feof may be true even though N > 0.
			   Otherwise, we could end up calling fread after EOF.  */
			if (feof(stream))
				goto process_partial_block;
		}

		/* Process buffer with BLOCKSIZE bytes.  Note that
		   BLOCKSIZE % 64 == 0
		 */
		sha256_process_block(buffer, BLOCKSIZE, &ctx);
	}

process_partial_block:;

	/* Process any remaining bytes.  */
	if (sum > 0)
		sha256_process_bytes(buffer, sum, &ctx);

	/* Construct result in desired memory.  */
	sha256_finish_ctx(&ctx, resblock);
	return 0;
}

/* Compute SHA512 message digest for LEN bytes beginning at BUFFER.  The
   result is always in little endian byte order, so that a byte-wise
   output yields to the wanted ASCII representation of the message
   digest.  */
void *sha256_buffer(const char *buffer, size_t len, void *resblock)
{
	struct sha256_ctx ctx;

	/* Initialize the computation context.  */
	sha256_init_ctx(&ctx);

	/* Process whole buffer but last len % 64 bytes.  */
	sha256_process_bytes(buffer, len, &ctx);

	/* Put result in desired memory area.  */
	return sha256_finish_ctx(&ctx, resblock);
}

void
sha256_process_bytes(const void *buffer, size_t len, struct sha256_ctx *ctx)
{
	/* When we already have some bits in our internal buffer concatenate
	   both inputs first.  */
	if (ctx->buflen != 0) {
		size_t left_over = ctx->buflen;
		size_t add = 128 - left_over > len ? len : 128 - left_over;

		memcpy(&((char *)ctx->buffer)[left_over], buffer, add);
		ctx->buflen += add;

		if (ctx->buflen > 64) {
			sha256_process_block(ctx->buffer, ctx->buflen & ~63,
					     ctx);

			ctx->buflen &= 63;
			/* The regions in the following copy operation cannot overlap.  */
			memcpy(ctx->buffer,
			       &((char *)ctx->buffer)[(left_over + add) & ~63],
			       ctx->buflen);
		}

		buffer = (const char *)buffer + add;
		len -= add;
	}

	/* Process available complete blocks.  */
	if (len >= 64) {
#if !_STRING_ARCH_unaligned
#define alignof(type) offsetof (struct { char c; type x; }, x)
#define UNALIGNED_P(p) (((size_t) p) % alignof (uint32_t) != 0)
		if (UNALIGNED_P(buffer))
			while (len > 64) {
				sha256_process_block(memcpy
						     (ctx->buffer, buffer, 64),
						     64, ctx);
				buffer = (const char *)buffer + 64;
				len -= 64;
		} else
#endif
		{
			sha256_process_block(buffer, len & ~63, ctx);
			buffer = (const char *)buffer + (len & ~63);
			len &= 63;
		}
	}

	/* Move remaining bytes in internal buffer.  */
	if (len > 0) {
		size_t left_over = ctx->buflen;

		memcpy(&((char *)ctx->buffer)[left_over], buffer, len);
		left_over += len;
		if (left_over >= 64) {
			sha256_process_block(ctx->buffer, 64, ctx);
			left_over -= 64;
			memcpy(ctx->buffer, &ctx->buffer[16], left_over);
		}
		ctx->buflen = left_over;
	}
}

/* --- Code below is the primary difference between sha1.c and sha256.c --- */

/* SHA256 round constants */
#define K(I) sha256_round_constants[I]
static const uint32_t sha256_round_constants[64] = {
	0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL,
	0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
	0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
	0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
	0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
	0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
	0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL,
	0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
	0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
	0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
	0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL,
	0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
	0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL,
	0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
	0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
	0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL,
};

/* Round functions.  */
#define F2(A,B,C) ( ( A & B ) | ( C & ( A | B ) ) )
#define F1(E,F,G) ( G ^ ( E & ( F ^ G ) ) )

/* Process LEN bytes of BUFFER, accumulating context into CTX.
   It is assumed that LEN % 64 == 0.
   Most of this code comes from GnuPG's cipher/sha1.c.  */

void
sha256_process_block(const void *buffer, size_t len, struct sha256_ctx *ctx)
{
	const uint32_t *words = buffer;
	size_t nwords = len / sizeof(uint32_t);
	const uint32_t *endp = words + nwords;
	uint32_t x[16];
	uint32_t a = ctx->state[0];
	uint32_t b = ctx->state[1];
	uint32_t c = ctx->state[2];
	uint32_t d = ctx->state[3];
	uint32_t e = ctx->state[4];
	uint32_t f = ctx->state[5];
	uint32_t g = ctx->state[6];
	uint32_t h = ctx->state[7];

	/* First increment the byte count.  FIPS PUB 180-2 specifies the possible
	   length of the file up to 2^64 bits.  Here we only compute the
	   number of bytes.  Do a double word increment.  */
	ctx->total[0] += len;
	if (ctx->total[0] < len)
		++ctx->total[1];

#define rol(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define S0(x) (rol(x,25)^rol(x,14)^(x>>3))
#define S1(x) (rol(x,15)^rol(x,13)^(x>>10))
#define SS0(x) (rol(x,30)^rol(x,19)^rol(x,10))
#define SS1(x) (rol(x,26)^rol(x,21)^rol(x,7))

#define M(I) ( tm =   S1(x[(I-2)&0x0f]) + x[(I-7)&0x0f] \
		    + S0(x[(I-15)&0x0f]) + x[I&0x0f]    \
	       , x[I&0x0f] = tm )

#define R(A,B,C,D,E,F,G,H,K,M)  do { t0 = SS0(A) + F2(A,B,C); \
                                     t1 = H + SS1(E)  \
                                      + F1(E,F,G)     \
				      + K	      \
				      + M;	      \
				     D += t1;  H = t0 + t1; \
			       } while(0)

	while (words < endp) {
		uint32_t tm;
		uint32_t t0, t1;
		int t;
		/* FIXME: see sha1.c for a better implementation.  */
		for (t = 0; t < 16; t++) {
			x[t] = SWAP(*words);
			words++;
		}

		R(a, b, c, d, e, f, g, h, K(0), x[0]);
		R(h, a, b, c, d, e, f, g, K(1), x[1]);
		R(g, h, a, b, c, d, e, f, K(2), x[2]);
		R(f, g, h, a, b, c, d, e, K(3), x[3]);
		R(e, f, g, h, a, b, c, d, K(4), x[4]);
		R(d, e, f, g, h, a, b, c, K(5), x[5]);
		R(c, d, e, f, g, h, a, b, K(6), x[6]);
		R(b, c, d, e, f, g, h, a, K(7), x[7]);
		R(a, b, c, d, e, f, g, h, K(8), x[8]);
		R(h, a, b, c, d, e, f, g, K(9), x[9]);
		R(g, h, a, b, c, d, e, f, K(10), x[10]);
		R(f, g, h, a, b, c, d, e, K(11), x[11]);
		R(e, f, g, h, a, b, c, d, K(12), x[12]);
		R(d, e, f, g, h, a, b, c, K(13), x[13]);
		R(c, d, e, f, g, h, a, b, K(14), x[14]);
		R(b, c, d, e, f, g, h, a, K(15), x[15]);
		R(a, b, c, d, e, f, g, h, K(16), M(16));
		R(h, a, b, c, d, e, f, g, K(17), M(17));
		R(g, h, a, b, c, d, e, f, K(18), M(18));
		R(f, g, h, a, b, c, d, e, K(19), M(19));
		R(e, f, g, h, a, b, c, d, K(20), M(20));
		R(d, e, f, g, h, a, b, c, K(21), M(21));
		R(c, d, e, f, g, h, a, b, K(22), M(22));
		R(b, c, d, e, f, g, h, a, K(23), M(23));
		R(a, b, c, d, e, f, g, h, K(24), M(24));
		R(h, a, b, c, d, e, f, g, K(25), M(25));
		R(g, h, a, b, c, d, e, f, K(26), M(26));
		R(f, g, h, a, b, c, d, e, K(27), M(27));
		R(e, f, g, h, a, b, c, d, K(28), M(28));
		R(d, e, f, g, h, a, b, c, K(29), M(29));
		R(c, d, e, f, g, h, a, b, K(30), M(30));
		R(b, c, d, e, f, g, h, a, K(31), M(31));
		R(a, b, c, d, e, f, g, h, K(32), M(32));
		R(h, a, b, c, d, e, f, g, K(33), M(33));
		R(g, h, a, b, c, d, e, f, K(34), M(34));
		R(f, g, h, a, b, c, d, e, K(35), M(35));
		R(e, f, g, h, a, b, c, d, K(36), M(36));
		R(d, e, f, g, h, a, b, c, K(37), M(37));
		R(c, d, e, f, g, h, a, b, K(38), M(38));
		R(b, c, d, e, f, g, h, a, K(39), M(39));
		R(a, b, c, d, e, f, g, h, K(40), M(40));
		R(h, a, b, c, d, e, f, g, K(41), M(41));
		R(g, h, a, b, c, d, e, f, K(42), M(42));
		R(f, g, h, a, b, c, d, e, K(43), M(43));
		R(e, f, g, h, a, b, c, d, K(44), M(44));
		R(d, e, f, g, h, a, b, c, K(45), M(45));
		R(c, d, e, f, g, h, a, b, K(46), M(46));
		R(b, c, d, e, f, g, h, a, K(47), M(47));
		R(a, b, c, d, e, f, g, h, K(48), M(48));
		R(h, a, b, c, d, e, f, g, K(49), M(49));
		R(g, h, a, b, c, d, e, f, K(50), M(50));
		R(f, g, h, a, b, c, d, e, K(51), M(51));
		R(e, f, g, h, a, b, c, d, K(52), M(52));
		R(d, e, f, g, h, a, b, c, K(53), M(53));
		R(c, d, e, f, g, h, a, b, K(54), M(54));
		R(b, c, d, e, f, g, h, a, K(55), M(55));
		R(a, b, c, d, e, f, g, h, K(56), M(56));
		R(h, a, b, c, d, e, f, g, K(57), M(57));
		R(g, h, a, b, c, d, e, f, K(58), M(58));
		R(f, g, h, a, b, c, d, e, K(59), M(59));
		R(e, f, g, h, a, b, c, d, K(60), M(60));
		R(d, e, f, g, h, a, b, c, K(61), M(61));
		R(c, d, e, f, g, h, a, b, K(62), M(62));
		R(b, c, d, e, f, g, h, a, K(63), M(63));

		a = ctx->state[0] += a;
		b = ctx->state[1] += b;
		c = ctx->state[2] += c;
		d = ctx->state[3] += d;
		e = ctx->state[4] += e;
		f = ctx->state[5] += f;
		g = ctx->state[6] += g;
		h = ctx->state[7] += h;
	}
}




char *file_sha256sum_alloc(const char *file_name)
{
	static const int sha256sum_bin_len = 32;
	static const int sha256sum_hex_len = 64;

	static const unsigned char bin2hex[16] = {
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'a', 'b',
		'c', 'd', 'e', 'f'
	    };

	int i, err;
	FILE *file;
	char *sha256sum_hex;
	unsigned char sha256sum_bin[sha256sum_bin_len];

	sha256sum_hex = xcalloc(1, sha256sum_hex_len + 1);

	file = fopen(file_name, "r");
	if (file == NULL) {
		targz_perror(ERROR, "Failed to open file %s", file_name);
		free(sha256sum_hex);
		return NULL;
	}

	err = sha256_stream(file, sha256sum_bin);
	if (err) {
		targz_msg(ERROR, "Could't compute sha256sum for %s.\n",
			 file_name);
		fclose(file);
		free(sha256sum_hex);
		return NULL;
	}

	fclose(file);

	for (i = 0; i < sha256sum_bin_len; i++) {
		sha256sum_hex[i * 2] = bin2hex[sha256sum_bin[i] >> 4];
		sha256sum_hex[i * 2 + 1] = bin2hex[sha256sum_bin[i] & 0xf];
	}

	sha256sum_hex[sha256sum_hex_len] = '\0';

	return sha256sum_hex;
}


