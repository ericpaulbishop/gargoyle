/* --------------------------------------------------------------------------
 * Copyright 2003-2014 (inclusive) Nathan Angelacos 
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
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "h_error.h"


/* we define this here, but should use the header files instead. */
void *xrealloc (void *buf, size_t size);


/*
 * split a string into an argv[] array, and return the number of elements.
 * Warning:  Overwrites instr with nulls (to make ASCIIZ strings) and mallocs
 * space for the argv array.  The argv array will point to the offsets in
 * instr where the elements occur.  The calling function must free the argv
 * array, and should do so before freeing instr memory.
 *
 * If comment points to a non-null string, then any character in the string
 * will mark the beginning of a comment until the end-of-line:
 *
 * comment="#;"
 * foo bar     # This is a comment
 * foo baz     ; This is a comment as well
 *
 * Example of use:
 *
 * int argc, count; argv_t *argv; char string[2000];
 *
 * strcpy (string, "This\\ string will be \"separated into\" '6' elements.", "");
 * argc = argc_argv (string, &argv);
 * for (count = 0; count < argc; count++) {
 * 	printf ("%03d: %s\n", count, argv[count].string);
 * 	}
 * free (argv);
 *
 */


int
argc_argv (char *instr, argv_t ** argv, char *commentstr)
{
  char quote = '\0';
  int arg_count = 0;
  enum state_t
  {
    WHITESPACE, WORDSPACE, TOKENSTART
  } state = WHITESPACE;
  argv_t *argv_array = NULL;
  int argc_slots = 0;
  size_t len, pos;

  len = strlen (instr);
  pos = 0;
  char quoted = 0;

  while (pos < len)
    {

      // printf ("%3d of %3d: %s\n", pos, len, instr);
 
      /* Comments are really, really special */
      if ((state == WHITESPACE) && (strchr (commentstr, *instr)))
	{
	  while ((*instr != '\n') && (*instr != '\0'))
	    {
	      instr++;
	      pos++;
	    }
	}

      switch (*instr)
	{

	  /* quoting */
	case '"':
	case '\'':
	  /* Begin quoting */
	  if (state == WHITESPACE)
	    {
	      quote = *instr;
	      state = TOKENSTART;
	      quoted = -1;
	      if (*(instr + 1) == quote)
		{		/* special case for NULL quote */
		  *instr = '\0';
		}
	      else
		{
		  instr++;
		  pos++;
		}
	    }
	  else
	    {			/* WORDSPACE, so quotes end or quotes within quotes */
	      /* Is it the same kind of quote? */
	      if ( (*instr == quote) && quoted )
		{
		  quote = '\0';
		  *instr = '\0';
		  state = WHITESPACE;
		}
	    }
	  break;

	  /* backslash - if escaping a quote within a quote  */
	case '\\':
	  if ((quote) && (*(instr + 1) == quote))
	    {
	      memmove (instr, instr + 1, strlen (instr));
	      len--;
	    }
	  /* otherwise, its just a normal character */
	  else
	    {
	      if (state == WHITESPACE)
		{
		  state = TOKENSTART;
		}
	    }
	  break;


	  /* whitepsace */
	case ' ':
	case '\t':
	case '\r':
	case '\n':
	  if ((state == WORDSPACE) && (quote == '\0'))
	    {
	      state = WHITESPACE;
	      *instr = '\0';
	    }
	  break;

	case '\0':
	  break;

	default:
	  if (state == WHITESPACE)
	    {
	      state = TOKENSTART;
	    }

	}			/* end switch */

      if (state == TOKENSTART)
	{
	  arg_count++;
	  if (arg_count > argc_slots)
	    {
	      argc_slots += ALLOC_CHUNK;
	      argv_array =
		(argv_t *) xrealloc (argv_array,
				     sizeof (argv_t) * (argc_slots +
							ALLOC_CHUNK));
	    }

	  if (argv_array == NULL)
	    {
	      return (-1);
	    }
	  argv_array[arg_count - 1].string = instr;
	  argv_array[arg_count - 1].quoted = quoted;
	  state = WORDSPACE;
	  quoted = 0;
	}

      instr++;
      pos++;
    }

  if ( arg_count == 0 ) return (0);

  argv_array[arg_count].string = NULL;
  *argv = argv_array;
  return (arg_count);
}

/* Expandable Buffer is a reimplementation based on buffer.c in GCC 
   originally by Per Bother */

void
haserl_buffer_init (buffer_t * buf)
{
  buf->data = NULL;
  buf->ptr = NULL;
  buf->limit = NULL;
}

void
buffer_destroy (buffer_t * buf)
{
  if (buf->data)
    {
      free (buf->data);
    }
  haserl_buffer_init (buf);
}

/* don't reallocate - just forget about the current contents */
void
buffer_reset (buffer_t * buf)
{
  if (buf->data)
    {
      buf->ptr = buf->data;
    }
  else
    {
      buf->ptr = NULL;
    }
}


void
buffer_add (buffer_t * buf, const void *data, unsigned long size)
{
  unsigned long newsize;
  unsigned long index;

  /* if we need to grow the buffer, do so now */
  if ((buf->ptr + size) >= buf->limit)
    {
      index = (buf->limit - buf->data);
      newsize = index;
      while (newsize <= index + size)
	{
	  newsize += 1024;
	}
      index = buf->ptr - buf->data;
      buf->data = realloc (buf->data, newsize);
	if ( buf->data == NULL ) 
          {
	   die_with_message ( NULL, NULL, "Memory allocation error");
          }
      buf->limit = buf->data + newsize;
      buf->ptr = buf->data + index;
    }

  memcpy (buf->ptr, data, size);
  buf->ptr += size;
}

#ifndef JUST_LUACSHELL

/* uppercase an entire string, using toupper */
void
uppercase (char *instr)
{
  while (*instr != '\0')
    {
      *instr = toupper (*instr);
      instr++;
    }
}


/* lowercase an entire string, using tolower */
void
lowercase (char *instr)
{
  while (*instr != '\0')
    {
      *instr = tolower (*instr);
      instr++;
    }
}

/* return ptr to first non-whitespace character */
char *
skip_whitespace (char *instr)
{
  while (isspace (*instr) && *instr)
    instr++;
  return instr;
}


/* return ptr to first whitespace character */
char *
find_whitespace (char *instr)
{
  while (!isspace (*instr) && *instr)
    instr++;
  return instr;
}



/* Counts the number of newlines in a buffer */
int
count_lines (char *instr, size_t len, char *where)
{
  size_t line = 1;
  while ((where > instr) && (len))
    {
      if (*instr == '\n')
	line++;
      len--;
      instr++;
    }
  return line;
}

#endif

#ifdef TEST_FRAMEWORK

main ()
{

  int argc, count;
  argv_t *argv;
  char string[2000];


  strcpy (string,
	  "\\This\\ string will be  '' \"separated into\"  \"'\\\"'\" ' 16 ' elements.\n"
	  "' including a multi-line\n"
	  "element' with a comment.  # This should not be parsed\n"
	  ";Nor should this\n" "The End.");


  argc = argc_argv (string, &argv, "#;");
  printf ("%s\n", string);

  for (count = 0; count < argc; count++)
    {
      printf ("%03d: [%s] ", count, argv[count].string, "");
      if (argv[count].quoted)
	{
	  printf ("(it was quoted)");
	}
      printf ("\n");
    }
  if (argc != 15)
    {
      puts ("Test FAILED");
    }
  else
    {
      puts ("Test PASSED");
    }
  free (argv);
}

#endif
