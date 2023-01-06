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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "common.h"
#include "h_luac.h"
#include "h_error.h"

extern lua_State *lua_vm;

	/* tokenizer was not used, so script is NULL, but name is the filename to process */
void
luac_doscript (buffer_t * script, char *name)
{
  if (luaL_loadfile (lua_vm, name) || lua_pcall (lua_vm, 0, LUA_MULTRET, 0))
    {
      die_with_message (NULL, NULL, "Cannot load lua and execute chunk: %s",
			lua_tostring (lua_vm, -1));
    }
}

int
h_luac_loadfile (lua_State * L)
{
  const char *filename = luaL_checkstring (L, 1);

  if (luaL_loadfile (L, filename))
    {
      die_with_message (NULL, NULL, "Cannot load file '%s': %s", filename,
			lua_tostring (L, -1));
    }				/* no error: function is on the stack */

  return 1;
}
