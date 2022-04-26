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



#ifndef H_ERROR_H
#define H_ERROR_H	1


enum error_types { E_NO_ERROR, E_MALLOC_FAIL, E_FILE_OPEN_FAIL,
		   E_END_BEFORE_BEGIN, E_NO_END_MARKER ,
		   E_NO_OP, E_SUBSHELL_FAIL, E_WHATEVER };

extern char *g_err_msg[];

/* h_error.c */
void die_with_error(char *msg);
void die_with_syntax(void *script, char *where, int error);
void
die_with_message ( void *sp, char *where, const char *s,  ...);
#endif /* !H_ERROR_H */
