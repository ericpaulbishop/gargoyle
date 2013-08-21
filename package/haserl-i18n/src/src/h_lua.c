/* --------------------------------------------------------------------------
 * lua language specific functions
 * Copyright (c) 2003-2007   Nathan Angelacos (nangel@users.sourceforge.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
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
#include <getopt.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "common.h"
#include "h_error.h"
#include "h_lua.h"
#include "h_script.h"
#include "haserl.h"

extern lua_State *lua_vm;

/* attempts to open a file, tokenize and then process it as a haserl script */
int
h_lua_loadfile (lua_State * L)
{
  script_t *scriptchain;
  token_t *tokenchain;
  buffer_t script_text;
  int status;

  /* get the filename */
  const char *filename = luaL_checkstring (L, 1);


  scriptchain = load_script ((char *) filename, NULL);
  tokenchain = build_token_list (scriptchain, NULL);
  preprocess_token_list (tokenchain);
  process_token_list (&script_text, tokenchain);
  free_token_list (tokenchain);
  free_script_list (scriptchain);

  /* script_text has the include file */
  status = luaL_loadbuffer (L, (char *) script_text.data,
			    script_text.ptr - script_text.data, filename);
  buffer_destroy (&script_text);
  if (status)
    {
      lua_error (L);
    }
  return (1);			/* we return one value, the buffer, as a function */
}

void
lua_exec (buffer_t * buf, char *str)
{
  buffer_add (buf, str, strlen (str));
}

void
lua_doscript (buffer_t * script, char *name)
{
  int status;
  /* force the string to be null terminated */

  buffer_add (script, "\0", 1);

  status = luaL_loadbuffer (lua_vm, (char *) script->data,
			    strlen ((char *) script->data), name) ||
    lua_pcall (lua_vm, 0, LUA_MULTRET, 0);

  if (status && !lua_isnil (lua_vm, -1))
    {
      const char *msg = lua_tostring (lua_vm, -1);
      if (msg == NULL)
	msg = "(error object is not a string)";
      die_with_message (NULL, NULL, msg);
    }
}


/* Run the echo command in a subshell */
void
lua_echo (buffer_t * buf, char *str, size_t len)
{
  static char echo_start[] = " io.write ";
  char quote[200] = "]=]";	/* 197 nested comments is a problem */

  if (len == 0)
    return;

  /* figure out if the string uses ]] ]=] ]==] etc in it */
  while ((strstr (str, quote)) && (strlen (quote) < 198))
    {
      memmove (quote + strlen (quote) - 1, quote + strlen (quote) - 2, 3);
    }

  /* As of 5.1, nested comments are depreciated... sigh */
  quote[0] = '[';
  quote[strlen (quote) - 1] = quote[0];
  while ((strstr (str, quote)) && (strlen (quote) < 198))
    {
      memmove (quote + strlen (quote) - 1, quote + strlen (quote) - 2, 3);
    }

  buffer_add (buf, echo_start, strlen (echo_start));
  buffer_add (buf, quote, strlen (quote));
  buffer_add (buf, str, len);
  quote[0] = ']';
  quote[strlen (quote) - 1] = quote[0];
  buffer_add (buf, quote, strlen (quote));
  buffer_add (buf, "\n", 1);

}


/* do an evaluation */
void
lua_eval (buffer_t * buf, char *str, size_t len)
{
  static char start[] = " io.write(tostring(";
  static char end[] = "))\n";
  if (len == 0)
    return;

  buffer_add (buf, start, strlen (start));
  buffer_add (buf, str, len);
  buffer_add (buf, end, strlen (end));
}

