PACKAGE=@PACKAGE_TARNAME@

EXTRA_DIST = haserl_lualib.lua lua2c.c

datarootdir=@datarootdir@

# Not strictly required, but we explicitly do not distribute the generated inc file
nodist_haserl_SOURCES = haserl_lualib.inc


if USE_LUA
noinst_PROGRAMS = lua2c
LUASOURCE = h_lua_common.c h_lua_common.h
if INCLUDE_LUASHELL
LUASOURCE += h_lua.c h_lua.h
endif
if INCLUDE_LUACSHELL
LUASOURCE += h_luac.c h_luac.h
endif
endif

if INCLUDE_BASHSHELL
BASHSOURCE = h_bash.c h_bash.h
endif

if USE_LUA
# lua2c_LIBS ?= -llua -ldl -lm
lua2c_LDFLAGS ?= -Wl,-E
lua2c:	lua2c.c
	$(CC_FOR_BUILD) $(CFLAGS) $(LUA_CFLAGS) $(LDFLAGS) $(lua2c_LDFLAGS) \
		-o $@ $^ $(LIBS) $(LUA_LIBS)

haserl_lualib.inc : haserl_lualib.lua lua2c
	if ! ./lua2c haserl_lualib haserl_lualib.lua >$@; then \
		rm $@; \
		exit 1 ;\
	fi

h_lua_common.c : haserl_lualib.inc

endif

bin_PROGRAMS = haserl

haserl_SOURCES = common.c common.h sliding_buffer.c sliding_buffer.h \
		 h_error.c h_error.h h_script.c h_script.h rfc2388.c rfc2388.h \
		 $(BASHSOURCE) $(LUASOURCE) h_translate.c h_translate.h haserl.c haserl.h
haserl_CFLAGS = $(LUA_CFLAGS)
haserl_LDADD = $(LUA_LIBS)

install-strip:
	$(MAKE) $(AM_MAKEFLAGS) INSTALL_PROGRAM="$(INSTALL_STRIP_PROGRAM)" \
	  install_sh_PROGRAM="$(INSTALL_STRIP_PROGRAM)" INSTALL_STRIP_FLAG=-s \
	  `test -z '$(STRIP)' || \
	    echo "INSTALL_PROGRAM_ENV=STRIPPROG='$(STRIP)'"` install

