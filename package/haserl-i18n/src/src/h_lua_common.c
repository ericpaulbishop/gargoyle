/* --------------------------------------------------------------------------
 * functions shared among both Lua shells
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

#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "common.h"
#include "h_lua_common.h"
#ifdef INCLUDE_LUASHELL
#include "h_lua.h"
#endif
#ifdef INCLUDE_LUACSHELL
#include "h_luac.h"
#endif
#include "h_error.h"

/* this is not a mistake.  We are including the 
 * definition of the lualib here 
 */
#include "haserl_lualib.inc"

lua_State *lua_vm = NULL;

void
lua_common_putenv (char *str)
{
  char *value;
  value = memchr (str, '=', strlen (str));
  if (value)
    {
      *value = (char) '\0';
      value++;
    }
  else
    {
      value = str + strlen (str);
    }


  lua_getglobal (lua_vm, "haserl");
  lua_pushstring (lua_vm, "myputenv");
  lua_gettable (lua_vm, -2);
  lua_pushstring (lua_vm, str);
  lua_pushstring (lua_vm, value);
  lua_call (lua_vm, 2, 0);
  return;
}

void
lua_common_setup (char *shell, list_t * env)
{
  /*  create a lua instance */
  lua_vm = luaL_newstate ();
  luaL_openlibs (lua_vm);

  /* and load our haserl library */
  if (luaL_loadbuffer
      (lua_vm, (const char *) &haserl_lualib, sizeof (haserl_lualib),
       "luascript.lua") || lua_pcall (lua_vm, 0, 0, 0))
    {
      die_with_message (NULL, NULL,
			"Error passing the lua library to the lua vm: %s",
			lua_tostring (lua_vm, -1));
    }

  /* and put the vars in the vm */
  while (env)
    {
      lua_common_putenv (env->buf);
      env = env->next;
    }


  /* register our open function in the haserl table */
  lua_getglobal (lua_vm, "haserl");
  lua_pushstring (lua_vm, "loadfile");
#if defined(INCLUDE_LUASHELL) && defined(INCLUDE_LUACSHELL)
  lua_pushcfunction (lua_vm,
		     shell[3] == 'c' ? h_luac_loadfile : h_lua_loadfile);
#elif defined(INCLUDE_LUASHELL)
  lua_pushcfunction (lua_vm, h_lua_loadfile);
#else /* INCLUDE_LUACSHELL */
  lua_pushcfunction (lua_vm, h_luac_loadfile);
#endif
  lua_settable (lua_vm, -3);

}

void
lua_common_destroy (void)
{
  /* close the lua instance */
  lua_close (lua_vm);
}
