PACKAGE=@PACKAGE_TARNAME@

EXTRA_DIST = haserl_lualib.lua haserl_lualib.inc lua2c.c

datarootdir=@datarootdir@

if USE_LUA
LUASOURCE = haserl_lualib.inc h_lua_common.c h_lua_common.h
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

haserl_lualib.inc : haserl_lualib.lua 
	@echo '-----------------------------------------------------'
	@echo 'Whoops.   haserl_lualib.inc is old.  You will need'
	@echo 'to compile lua2c by hand, or help the maintainer'
	@echo 'get automake to do it for you.'
	@echo ''
	@echo 'For now, to compile lua2c:'
	@echo ''
	@echo 'gcc -I<luaheaderdir> -Wl,-E -L<lualibdir> -o lua2c lua2c.c -llua -ldl -lm'
	@echo ''
	@echo ''
	@echo 'Then follow the instructions in lua2c.c to create a'
	@echo 'new haserl_lualib.inc'
	@echo ''
	@echo 'Sorry.'
	@echo '-----------------------------------------------------'
	@exit 1

h_lua_common.c : haserl_lualib.inc

endif

bin_PROGRAMS = haserl

haserl_SOURCES = common.c common.h sliding_buffer.c sliding_buffer.h \
		 h_error.c h_error.h h_script.c h_script.h rfc2388.c rfc2388.h \
		 $(BASHSOURCE) $(LUASOURCE) h_translate.c h_translate.h haserl.c haserl.h

install-strip:
	$(MAKE) $(AM_MAKEFLAGS) INSTALL_PROGRAM="$(INSTALL_STRIP_PROGRAM)" \
          install_sh_PROGRAM="$(INSTALL_STRIP_PROGRAM)" INSTALL_STRIP_FLAG=-s \
          `test -z '$(STRIP)' || \
            echo "INSTALL_PROGRAM_ENV=STRIPPROG='$(STRIP)'"` install


#---------  Unit Tests ----------------------
#TESTS = utest_common

#check_PROGRAMS = utest_common

#utest_common: utest_common.c

#--------- End Unit Tests -------------------

