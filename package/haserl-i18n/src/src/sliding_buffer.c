/*
 * --------------------------------------------------------------------------
 * Sliding Buffer functions for haserl Copyright (c) 2007 Nathan Angelacos
 * (nangel@users.sourceforge.net) This program is free software; you can
 * redistribute it and/or modify it under the terms of the GNU General Public
 * License, version 2, as published by the Free Software Foundation. This
 * program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details. You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * ------------------------------------------------------------------------- 
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#include "sliding_buffer.h"

/*
 * initialize a sliding buffer structure 
 */
int
s_buffer_init (sliding_buffer_t * sbuf, int size)
{
  sbuf->maxsize = size;
  sbuf->buf = malloc (sbuf->maxsize);
  /* reduce maxsize by one, so that you can add a NULL to the end of any
     returned token and not have a memory overwrite */
  sbuf->maxsize -= 1;
  sbuf->fh = 0;			/* use stdin by default */
  sbuf->eof = 0;
  sbuf->len = 0;
  sbuf->ptr = sbuf->buf;
  sbuf->bufsize = 0;
  sbuf->maxread = 0;
  sbuf->nrread = 0;
  return (sbuf->buf != NULL);	/* return true if the alloc succeeded */
}

/*
 * destroy a sliding buffer structure 
 */
void
s_buffer_destroy (sliding_buffer_t * sbuf)
{
  free (sbuf->buf);
}


/*
 * read the next segment from a sliding buffer. returns !=0 if the
 * segment ends at a matchstr token, or if we are at the end of the string
 *  returns 0 if the segment does not end 
 */
int
s_buffer_read (sliding_buffer_t * sbuf, char *matchstr)
{
  int len, pos;
  int r;

  /*
   * if eof and ptr ran off the buffer, then we are done 
   */
  if ((sbuf->eof) && (sbuf->ptr > sbuf->buf))
    {
      return 0;
    }

  /*
   * if need to fill the buffer, do so 
   */
  if ((sbuf->bufsize == 0) ||
      (sbuf->ptr >= (sbuf->buf + sbuf->bufsize - strlen (matchstr))))
    {
      len = sbuf->bufsize - (sbuf->ptr - sbuf->buf);
      if (len)
	{
	  memmove (sbuf->buf, sbuf->ptr, len);
	}
      sbuf->ptr = sbuf->buf;
      sbuf->bufsize = len;
      /* if the filedescriptor is invalid, we are obviously
       * at an end of file condition. 
       */
      if (fcntl (sbuf->fh, F_GETFL) == -1)
	{
	  r = 0;
	}
      else
	{
	  size_t n = sbuf->maxsize - len;
	  if ( sbuf->maxread && sbuf->maxread < sbuf->nrread + n)
	    n = sbuf->maxread - sbuf->nrread;
	  r = read (sbuf->fh, sbuf->buf + len, n);
	}
      /*
       * only report eof when we've done a read of 0.
       */
      if (r == 0 || (r < 0 && errno != EINTR))
	{
	  sbuf->eof = -1;
	}
      else
	{
	  sbuf->bufsize += (r > 0) ? r : 0;
	  sbuf->nrread += (r > 0) ? r : 0;
	}
    }

  /*
   * look for the matchstr 
   */
  pos = 0;
  len = sbuf->bufsize - (int) (sbuf->ptr - sbuf->buf) - strlen (matchstr);
  while (memcmp (matchstr, sbuf->ptr + pos, strlen (matchstr)) && (pos < len))
    {
      pos++;
    }

  /*
   * if we found it 
   */
  if (pos < len)
    {
      sbuf->len = pos;
      sbuf->segment = sbuf->ptr;
      sbuf->ptr = sbuf->segment + pos + strlen (matchstr);
      return -1;
    }

  if (sbuf->eof)
    {
      len += strlen (matchstr);
    }

  /*
   * ran off the end, didn't find the matchstr
   */
  sbuf->segment = sbuf->ptr;
  sbuf->len = len;
  sbuf->ptr += sbuf->len;
  return (sbuf->eof) ? (-1) : (0);
}



#ifdef TEST_FRAMEWORK

main ()
{
  int x;
  sliding_buffer_t sb;
  char foo[200];

  s_buffer_init (&sb, 32);

  do
    {
      x = s_buffer_read (&sb, "&");
      sprintf (foo, "%03d- %03d - %03d", x, sb.eof, sb.len);
      write (1, foo, strlen (foo));
      write (1, sb.segment, sb.len);
      write (1, "\n", 1);
    }
  while ((!sb.eof));
  s_buffer_destroy (&sb);
}

#endif
