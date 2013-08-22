/* --------------------------------------------------------------------------
 * multipart/form-data handler functions (obviously, see rfc2388 for info)
 * Copyright (c) 2007   Nathan Angelacos (nangel@users.sourceforge.net)
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by the Free
 * Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 ------------------------------------------------------------------------- */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "common.h"
#include <erics_tools.h>
#include "h_error.h"
#include "h_script.h"
#include "h_bash.h"
#include "sliding_buffer.h"
#include "rfc2388.h"

#include "haserl.h"

void
empty_stdin (void)
{
  char c[2000];
  while (read (STDIN_FILENO, &c, 2000))
    {
    };
}


void
mime_var_init (mime_var_t * obj)
{
  obj->name = NULL;
  obj->filename = NULL;
  obj->type = NULL;
  obj->tempname = NULL;
  haserl_buffer_init (&(obj->value));
  obj->fh = 0;
}

void
mime_var_destroy (mime_var_t * obj)
{
  int status;

  if (obj->name)
    {
      free (obj->name);
      obj->name = NULL;
    }
  if (obj->filename)
    {
      free (obj->filename);
      obj->filename = NULL;
    }
  if (obj->type)
    {
      free (obj->type);
      obj->type = NULL;
    }
  if (obj->tempname)
    {
      free (obj->tempname);
      obj->tempname = NULL;
    }
  buffer_destroy (&(obj->value));
  if (obj->fh)
    {
      close (obj->fh);
      if (global.uploadhandler)
	{
	  wait (&status);
	}
      obj->fh = 0;
    }
}

char *
mime_substr (char *start, int len)
{
  char *ptr;

  if (!start)
    return NULL;
  if (len < 0)
    return NULL;
  ptr = xmalloc (len + 2);
  memcpy (ptr, start, len);
  return ptr;

}

void
mime_tag_add (mime_var_t * obj, char *str)
{
  char *a = NULL;
  char *b = NULL;
  static char *tag[] = { "name=\"", "filename=\"", "Content-Type: " };

  a = strcasestr (str, tag[0]);
  if (a)
    {
      a += strlen (tag[0]);
      b = strchr (a, '"');
      if (!obj->name)
	obj->name = mime_substr (a, b - a);
    }

  a = strcasestr (str, tag[1]);
  if (a)
    {
      a += strlen (tag[1]);
      b = strchr (a, '"');
      if (!obj->filename)
	obj->filename = mime_substr (a, b - a);
    }

  a = strcasestr (str, tag[2]);
  if (a)
    {
      a += strlen (tag[2]);
      b = a + strlen (a);
      if (!obj->type)
	obj->type = mime_substr (a, b - a);
    }
}

void
mime_var_putenv (list_t * env, mime_var_t * obj)
{
  buffer_t buf;
  haserl_buffer_init (&buf);
  if (obj->name)
    {
      buffer_add (&(obj->value), "", 1);
      buffer_add (&buf, obj->name, strlen (obj->name));
      buffer_add (&buf, "=", 1);
      buffer_add (&buf, (char *) obj->value.data,
		  strlen ((char *) obj->value.data) + 1);
      myputenv (env, (char *) buf.data, global.var_prefix);
      myputenv (env, (char *) buf.data, global.post_prefix);
      buffer_reset (&buf);
    }
  if (obj->filename)
    {
      buffer_add (&buf, obj->name, strlen (obj->name));
      buffer_add (&buf, "_name=", 6);
      buffer_add (&buf, obj->filename, strlen (obj->filename) + 1);
      myputenv (env, (char *) buf.data, global.var_prefix);
      myputenv (env, (char *) buf.data, global.post_prefix);
      buffer_reset (&buf);
    }
  buffer_destroy (&buf);
}

void
mime_exec (mime_var_t * obj, char *fifo)
{

  int pid;
  char *av[4];
  char *type, *filename, *name;
  char *c;
  int fh;

  pid = fork ();
  if (pid == -1)
    {
      empty_stdin ();
      die_with_message (NULL, NULL, g_err_msg[E_SUBSHELL_FAIL]);
    }

  if (pid == 0)
    {

      /* store the content type, filename, and form name */
      /* we do not use global.var_prefix because it could be lua or shell
       * or something else, and we are only shell here */
      if (obj->type)
	{

	  type = xmalloc (13 + strlen (obj->type) + 1);
	  sprintf (type, "CONTENT_TYPE=%s", obj->type);
	  putenv (type);
	}
      if (obj->filename)
	{
	  filename = xmalloc (9 + strlen (obj->filename) + 1);
	  sprintf (filename, "FILENAME=%s", obj->filename);
	  putenv (filename);
	}

      if (obj->name)
	{
	  name = xmalloc (5 + strlen (obj->name) + 1);
	  sprintf (name, "NAME=%s", obj->name);
	  putenv (name);
	}

      av[0] = global.uploadhandler;
      av[1] = fifo;
      av[2] = NULL;
      execv (av[0], av);
      /* if we get here, we had a failure. Not much we can do. 
       * We are the child, so we can't even warn the parent */
      fh = open (fifo, O_RDONLY);
      while (read (fh, &c, 1))
	{
	}
      exit (-1);
    }
  else
    {
      /* I'm parent */
    }

  /* control should get to this point only in the parent.
   */
}				/* end mime_exec */

void
mime_var_open_target (mime_var_t * obj)
{
  char *tmpname;
  token_t *curtoken;
  curtoken = global.uploadlist;
  int ok;

  /* if upload_limit is zero, we die right here */
  if (global.uploadkb == 0)
    {
      empty_stdin ();
      die_with_message (NULL, NULL, "File uploads are not allowed.");
    }


  ok = -1;
  tmpname = xmalloc (strlen (global.uploaddir) + 8);
  strcpy (tmpname, global.uploaddir);
  strcat (tmpname, "/XXXXXX");
  obj->fh = mkstemp (tmpname);

  if (obj->fh == -1)
    {
      ok = 0;
    }

  /* reuse the name as a fifo if we have a handler.  We do this 
   * because tempnam uses TEMPDIR if defined, among other bugs
   */
  if ((ok) && global.uploadhandler)
    {

      /* I have a handler */
      close (obj->fh);
      unlink (tmpname);
      if (mkfifo (tmpname, 0600))
	ok = 0;
      /* you must open the fifo for reading before writing
       * on non linux systems
       */
      if (ok)
	{
	  mime_exec (obj, tmpname);
	  obj->fh = open (tmpname, O_WRONLY);
	}
      if (obj->fh == -1)
	ok = 0;
    }
  else
    {
      buffer_add (&(obj->value), tmpname, strlen (tmpname));
    }

  if (!ok)
    {
      empty_stdin ();
      die_with_message (NULL, NULL, g_err_msg[E_FILE_OPEN_FAIL], tmpname);
    }

  curtoken =
    push_token_on_list (curtoken, NULL, tmpname, strlen (tmpname) + 1);
  if (global.uploadlist == NULL)
    {
      global.uploadlist = curtoken;
    }

}




void
mime_var_writer (mime_var_t * obj, char *str, int len)
{
  /* if not a file upload, then just a normal variable */
  if (!obj->filename)
    {
      buffer_add (&(obj->value), str, len);
    }

  /* if a file upload, but don't have an open filehandle, open one */
  if ((!obj->fh) && (obj->filename))
    mime_var_open_target (obj);

  /* if we have an open file, write the chunk */
  if (obj->fh)
    {
      write (obj->fh, str, len);
    }
}

/*
 * Read multipart/form-data input (RFC2388), typically used when
 * uploading a file.
 */

int
rfc2388_handler (list_t * env)
{
  enum mime_state_t
  { DISCARD, BOUNDARY, HEADER, CONTENT };


  int state;
  int i, x;
  unsigned long max_len, content_length;
  sliding_buffer_t sbuf;
  char *crlf = "\r\n";
  char *boundary;
  char *str;
  buffer_t buf;
  mime_var_t var;

  /* get the boundary info */
  str = getenv ("CONTENT_TYPE");
  i = strlen (str) - 9;
  while ((i >= 0) && (memcmp ("boundary=", str + i, 9)))
    {
      i--;
    }
  if (i == -1)
    {
      empty_stdin ();
      die_with_message (NULL, NULL, "No Mime Boundary Information Found");
    }

  i = i + 9;
  if (str[i] == '"')
    i++;

  boundary = xmalloc (strlen (str + i) + 5);	/* \r\n-- + NULL */
  memcpy (boundary, crlf, 2);
  memcpy (boundary + 2, "--", 2);
  memcpy (boundary + 4, str + i, strlen (str + i) + 1);
  if ((i > 0) && (str[i - 1] == '"'))
    {
      while ((boundary[i]) && (boundary[i] != '"'))
	i++;
      boundary[i] = '\0';
    }

  /* Allow 2MB content, unless they have a global upload set */
  max_len = ((global.uploadkb == 0) ? 2048 : global.uploadkb) *1024;
  content_length = 0;

  /* initialize a 128K sliding buffer */
  s_buffer_init (&sbuf, 1024 * 128);
  sbuf.fh = STDIN;
  if (getenv ("CONTENT_LENGTH"))
    {
      sbuf.maxread = strtoul (getenv ("CONTENT_LENGTH"), NULL, 10);
    }

  /* initialize the buffer, and make sure it doesn't point to null */
  haserl_buffer_init (&buf);
  buffer_add (&buf, "", 1);
  buffer_reset (&buf);

  state = DISCARD;
  str = boundary + 2;		/* skip the leading crlf */
  do
    {
      /* x is true if this token ends with a matchstr or is at the end of stream */
      x = s_buffer_read (&sbuf, str);
      content_length += sbuf.len;
      if (content_length >= max_len)
	{
	  empty_stdin ();
	  free (boundary);
	  s_buffer_destroy (&sbuf);
	  buffer_destroy (&buf);
	  if (var.name)
	    {
	      mime_var_destroy (&var);
	    }
	  die_with_message (NULL, NULL,
			    "Attempted to send content larger than allowed limits.");
	}

      switch (state)
	{

	case DISCARD:
	  /* discard any text - used for first mime boundary */
	  if (x)
	    {
	      state = BOUNDARY;
	      str = crlf;
	      buffer_reset (&buf);	/* reinitializes the buffer */
	    }
	  break;


	case BOUNDARY:
	  if (!x)
	    {
	      buffer_add (&buf, sbuf.segment, sbuf.len);
	    }
	  if (x)
	    {
	      buffer_add (&buf, sbuf.segment, sbuf.len);
	      if (!memcmp (buf.data, boundary + 2, 2))
		{		/* "--" */
		  /* all done... what does that mean? */
		  str = boundary + 2;
		  state = DISCARD;
		}
	      else
		{
		  buffer_reset (&buf);
		  mime_var_init (&var);
		  state = HEADER;
		  str = crlf;
		}
	    }
	  break;

	case HEADER:
	  buffer_add (&buf, sbuf.segment, sbuf.len);
	  if (x)
	    {
	      if (sbuf.len == 0)
		{		/* blank line */
		  buffer_reset (&buf);
		  state = CONTENT;
		  str = boundary;
		}
	      else
		{
		  buffer_add (&buf, "", 1);
		  mime_tag_add (&var, (char *) buf.data);
		  buffer_reset (&buf);
		}
	    }
	  break;

	case CONTENT:
	  /* write to writer process, regardless */
	  mime_var_writer (&var, (char *) sbuf.segment, sbuf.len);
	  if (x)
	    {
	      buffer_reset (&buf);
	      mime_var_putenv (env, &var);
	      mime_var_destroy (&var);
	      state = BOUNDARY;
	      str = crlf;
	    }

	  break;

	}			/* end switch */

    }
  while (!sbuf.eof);
  free (boundary);
  s_buffer_destroy (&sbuf);
  buffer_destroy (&buf);
  return (0);
}
