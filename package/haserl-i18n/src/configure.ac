# Process this file with autoconf to produce a configure script.
AC_INIT([haserl],[0.9.26],[Nathan Angelacos <nangel@users.sourceforge.net>],[haserl])
AM_INIT_AUTOMAKE([haserl],[$PACKAGE_VERSION])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CONFIG_HEADERS(src/config.h)
AC_CHECK_HEADERS([stdlib.h string.h unistd.h signal.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MEMCMP
AC_CHECK_FUNCS([memset putenv strcasecmp strdup strncasecmp strstr])

AC_DEFINE([_GNU_SOURCE], [], [Enable GNU Extensions])


dnl **************************************************************
AC_ARG_WITH(lua-headers,
   [  --with-lua-headers=DIR  lua include files location],
   [LUA_HDR_DIR="$withval"]
   [CFLAGS="$CFLAGS -I$withval"] 
)


dnl Checks for lua

luashell=false
luacshell=false
ac_report_have_lua=disabled
ac_report_bash_extensions=disabled


AC_ARG_WITH(lua, 
	AC_HELP_STRING([--with-lua[[=DIR]]], [use lua in DIR]),
[ case "$withval" in
  no)	ac_report_have_lua=disabled
	;;	
  *)	AC_SEARCH_LIBS(dlopen, dl)
	# ubuntu has lua5.1 rather than just lua
	if pkg-config --exists lua5.1; then
		LUALIB=lua5.1
	else
		LUALIB=lua
	fi
	if test -z "$LUA_HDR_DIR"; then
		CFLAGS="$CFLAGS `pkg-config $LUALIB --cflags`"
	fi
	LIBS="$LIBS -lm"
	LDFLAGS="$LDFLAGS -Wl,-E -L$withval"
	AC_DEFINE(USE_LUA, , [Enable Lua])

	AC_CHECK_LIB($LUALIB, luaL_newstate, , [
        	AC_MSG_ERROR([The Lua runtime library cannot be found!])
	], $LIBS)
	luashell=true
	luacshell=true
	ac_report_have_lua=enabled
	;;
  esac ], [
  	ac_report_have_lua=disabled
])
AM_CONDITIONAL(USE_LUA, test x$ac_report_have_lua = xenabled)

# If Lua is enabled, the user can choose between two different shells

# shell: ordinary Haserl code with embedded Lua
AC_ARG_ENABLE(luashell,
	AC_HELP_STRING([--enable-luashell], [Includes the standard Lua shell - Haserl with embedded Lua (default is yes if Lua's enabled)]),
[case "${enableval}" in
  yes) luashell=true ;;
  no)  luashell=false ;;
  *)   AC_MSG_ERROR(bad value ${enableval} for --enable-luashell) ;;
esac], [])
AM_CONDITIONAL(INCLUDE_LUASHELL, test x$luashell = xtrue)
if test x$luashell = xtrue; then
	AC_DEFINE(INCLUDE_LUASHELL, , [Include ordinary Lua shell])
fi

# shell: precompiled Haserl code - compiled Lua code
AC_ARG_ENABLE(luacshell,
	AC_HELP_STRING([--enable-luacshell], [Includes the compiled Lua shell - precompiled Haserl/Lua (default is yes if Lua's enabled)]),
[case "${enableval}" in
  yes) luacshell=true ;;
  no)  luacshell=false ;;
  *)   AC_MSG_ERROR(bad value ${enableval} for --enable-luacshell) ;;
esac], [])
AM_CONDITIONAL(INCLUDE_LUACSHELL, test x$luacshell = xtrue)
if test x$luacshell = xtrue; then
	AC_DEFINE(INCLUDE_LUACSHELL, , [Include shell for precompiled Haserl/Lua])
fi

if test \(x$luashell = xtrue -o x$luacshell = xtrue\) -a x$ac_report_have_lua = xdisabled; then
	AC_MSG_ERROR([Lua is not enabled so you cannot build a Lua shell.])
fi

# the Linux shell is always available

AC_ARG_ENABLE(bashshell,
	AC_HELP_STRING([--enable-bashshell], [Includes the Bash or another Linux shell (default is yes)]),
[case "${enableval}" in
  yes) bashshell=true ;;
  no)  bashshell=false ;;
  *)   AC_MSG_ERROR(bad value ${enableval} for --enable-bashshell) ;;
esac], [bashshell=true])
AM_CONDITIONAL(INCLUDE_BASHSHELL, test x$bashshell = xtrue)
if test x$bashshell = xtrue; then
	AC_DEFINE(INCLUDE_BASHSHELL, , [Include Bash/Linux shell])
fi

# set additional macros

if test x$luashell = xfalse -a x$bashshell = xfalse; then
	if test x$luacshell = xtrue; then
		AC_DEFINE(JUST_LUACSHELL, , [Include just the compiled Lua shell])
	else
		AC_MSG_ERROR([All shells disabled.])
	fi
fi

if test x$ac_report_have_lua = xenabled -a x$luashell = xfalse -a x$luacshell = xfalse; then
	AC_MSG_ERROR([Cannot enable Lua if no Lua shell is included.])
fi


# Do we include scott's bash extensions
AC_ARG_ENABLE(bash-extensions,
	AC_HELP_STRING([--enable-bash-extensions], [Includes bash extensions to the haserl script language (default is no)]),
[case "${enableval}" in
  yes) bashextensions=true 
	ac_report_bash_extensions=enabled 
	;;
  no)  bashextensions=false 
	ac_report_bash_extensions=disabled
	;;
  *)   AC_MSG_ERROR(bad value ${enableval} for --enable-bash-extensions) ;;
esac], [bashextensions=false])
AM_CONDITIONAL(BASHEXTENSIONS, test x$bashextensions = xtrue)
if test x$bashextensions = xtrue; then
	AC_DEFINE(BASHEXTENSIONS, , [Include bash extensions])
fi


# What subshell should we use
AC_ARG_ENABLE(subshell,
	[  --enable-subshell=cmd - enable the subshell to use (default is /bin/sh)])

# Handle "--disable-subshell"
case $enable_subshell in
	no )	enable_subshell=
		;;
	"" )	enable_subshell="/bin/sh"
		;;
	esac

AC_DEFINE_UNQUOTED( [SUBSHELL_CMD], "$enable_subshell", 
	[the subshell to start up])

CFLAGS="$CFLAGS -Wall"

AC_CONFIG_FILES([Makefile
		src/Makefile
		doc/Makefile
		tests/Makefile
		])




AC_OUTPUT 

dnl  report configuration
AC_MSG_RESULT([
** Configuration summary for $PACKAGE $VERSION:

	Building haserl with with lua $ac_report_have_lua 
	Building haserl with with bash-extensions $ac_report_bash_extensions
	libs: $LIBS
])


