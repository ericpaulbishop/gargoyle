/* --------------------------------------------------------------------------
 * Copyright 2003-2011 (inclusive) Nathan Angelacos 
 *                   (nangel@users.sourceforge.net)
 * 
 *   This file is part of haserl.
 *
 *   Haserl is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   as published by the Free Software Foundation.
 *
 *   Haserl is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with haserl.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------------ */

#ifndef _SLIDING_BUF_H
#define _SLIDING_BUF_H	1


/* sliding buffer structure */
typedef struct {
	int		fh;			/* the input filehandle for the buffer */
	unsigned char	*buf;			/* pointer to the buffer  */
	unsigned char	*ptr;			/* start positon (used internally) */
	unsigned char	*segment;		/* the start position of this segment */ 
	size_t		len;			/* length of this segment */
	size_t		maxsize;		/* max size of buffer */
	size_t		bufsize;		/* current size of buffer */
	size_t		maxread;		/* maximum number of bytes to read from fh, ignored if 0 */
	size_t		nrread;			/* number of bytes read from fh */
	int		eof; 			/* true if there is no more to read */
	} sliding_buffer_t;



/* sliding_buffer.c */
int s_buffer_init(sliding_buffer_t *sbuf, int size);
void s_buffer_destroy(sliding_buffer_t *sbuf);
int s_buffer_read(sliding_buffer_t *sbuf, char *matchstr);


#endif /* !_SLIDING_BUF_H */
