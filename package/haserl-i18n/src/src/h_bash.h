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

#ifndef H_SUBSHELL_H
#define H_SUBSHELL_H      1


/* the "names" for the pipes to the subshell */
enum pipe_t { PARENT_IN, PARENT_OUT };

/* h_bash.c */
void bash_destroy(void);
void bash_exec(buffer_t *buf, char *str);
void bash_wait(buffer_t *buf, char *str);
void bash_echo(buffer_t *buf, char *str, size_t len);
void bash_eval(buffer_t *buf, char *str, size_t len);
void bash_setup(char *shell, list_t *env);
void bash_doscript(buffer_t *script, char *name);

#ifdef BASHEXTENSIONS
void bash_if(buffer_t *buf, char *str, size_t len);
void bash_elif(buffer_t *buf, char *str, size_t len);
void bash_else(buffer_t *buf, char *str, size_t len);
void bash_endif(buffer_t *buf, char *str, size_t len);
void bash_case(buffer_t *buf, char *str, size_t len);
void bash_when(buffer_t *buf, char *str, size_t len);
void bash_otherwise(buffer_t *buf, char *str, size_t len);
void bash_endcase(buffer_t *buf, char *str, size_t len);
void bash_while(buffer_t *buf, char *str, size_t len);
void bash_endwhile(buffer_t *buf, char *str, size_t len);
void bash_until(buffer_t *buf, char *str, size_t len);
void bash_enduntil(buffer_t *buf, char *str, size_t len);
void bash_for(buffer_t *buf, char *str, size_t len);
void bash_endfor(buffer_t *buf, char *str, size_t len);
void bash_unless(buffer_t *buf, char *str, size_t len);
void bash_elun(buffer_t *buf, char *str, size_t len);
void bash_unelse(buffer_t *buf, char *str, size_t len);
void bash_endunless(buffer_t *buf, char *str, size_t len);
#endif

#endif /* !H_SUBSHELL_H */
