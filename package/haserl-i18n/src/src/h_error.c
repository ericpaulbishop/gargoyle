/*
 * --------------------------------------------------------------------------
 * Error functions for haserl
 * Copyright (c) 2003-2006 Nathan Angelacos (nangel@users.sourceforge.net)
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by the Free
 * Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
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
#include <stdarg.h>

#include "common.h"
#include <erics_tools.h>
#include "h_script.h"
#include "haserl.h"
#include "h_error.h"

char *g_err_msg[] = {
  "",
  "Memory Allocation Failure",
  "Unable to open file %s",
  "%c&gt; before &lt;%c",
  "Missing %c&gt;",
  "Unknown operation",
  "Unable to start subshell",
  "Unspecified Error",
};



/*
 * abort the program
 */
void
die_with_error (char *msg)
{
  fprintf (stderr, "Error: %s\n", msg);
  exit (-1);
}


/* print an error message and die.  If sp or where are non-null pointers, then
   a line is added saying where in the script buffer the error occured.  If
   there's a request method, then http headers are added.
 */
void
die_with_message (void *sp, char *where, const char *s, ...)
{
  #ifndef JUST_LUACSHELL
  script_t *script = sp;
  #endif
  va_list p;
  FILE *fo = stderr;

  if (global.silent == FALSE)
    {
      if (getenv ("REQUEST_METHOD"))
	{
	  fo = stdout;
	  fprintf (fo, "HTTP/1.0 500 Server Error\n"
		   "Content-Type: text/html\n\n"
		   "<html><body><b><font color=#CC0000>" PACKAGE_NAME
		   " CGI Error</font></b><br><pre>\n");
	}
      va_start (p, s);
      vfprintf (fo, s, p);
      va_end (p);
      #ifndef JUST_LUACSHELL
      if (where && sp)
        {
	  fprintf (fo, " near line %d of %s\n",
		   count_lines (script->buf, script->size, where),
		   script->name);
	}
      #endif
      printf ("\n");

      if (getenv ("REQUEST_METHOD"))
	fprintf (fo, "</pre></body></html>\n");
    }
  exit (-1);

}
