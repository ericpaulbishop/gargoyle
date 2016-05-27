/* --------------------------------------------------------------------------
 * a simple lua 2 c function converter - a simple luac + bin2c
 * Copyright (c) 2007   Nathan Angelacos (nangel@users.sourceforge.net)
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
 * ------------------------------------------------------------------------- */


/* This program loads the source file, and then uses lua_dump to output it
 * to a c const char array.  Because lua_dump is used instead of the internal
 * luaU_dump function, debugging info is in the output array.   If you want
 * to compile the array without debugging information, first use luac on the
 * .lua source, and then run lua2c on that output:
 * luac -s -o foo haserl_lualib.lua
 * lua2c haserl_lualib foo >haserl_lualib.inc
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

lua_State *lua_vm = NULL;


static void
loadit (char *filename)
{

lua_vm = luaL_newstate();
luaL_openlibs (lua_vm);

if (luaL_loadfile(lua_vm, filename)) {
	puts (lua_tostring(lua_vm, -1));
	exit (-1);
	}
	
}

static int writer (lua_State* L, const void* p, size_t size, void* u)
{
	static int count = 0;
	int i;
	for (i=0; i < size; i++ ) {
		if ((count) && (count % 16) == 0 ) printf ("\n  ");
		printf ("%3d,", *((unsigned char *) (p+i)));
		count++;
		}

	return (0);
}


static void 
dumpit() 
{
lua_dump (lua_vm, writer, NULL);
}


int
main (int argc, char *argv[]) {
	if (argc != 3) {
		printf("usage: %s varname luasource >output\n", argv[0]);
		return (-1);
		}

	loadit (argv[2]);


	printf ("/* This file was automatically generated from %s. DO NOT EDIT */\n\n", argv[2]);
	printf ("static const unsigned char %s[] = { \n  ", argv[1]);
	dumpit();
	printf ("\n};\n");

	return (0);
}
