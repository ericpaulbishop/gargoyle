#ifndef _H_LUAC_H
#define _H_LUAC_H

void luac_doscript(buffer_t *script, char *name);
int h_luac_loadfile(lua_State * L);

#endif
