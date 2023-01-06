/* --------------------------------------------------------------------------
 * Copyright 2003-2021 (inclusive) Nathan Angelacos 
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

#ifndef H_SCRIPT_H
#define H_SCRIPT_H	1


/* Everything we care to know about a script */
typedef struct {
	char 	*name; 		/* pointer to name of script 	*/
	int	size;		/* size of script in bytes 	*/	
 	uid_t	uid;		/* user owner			*/
	gid_t	gid;		/* group owner			*/
	char 	*buf;		/* pointer to malloc'ed buffer	*/
	size_t	curpos;		/* current position in buffer	*/
	int	bang_script;	/* true if script starts with #!*/
	int	tokens;		/* number of tokens in script	*/
	void    *next;		/* next script in our chain     */
	} script_t;

/* tag types */
#ifdef BASHEXTENSIONS
enum tag_t { HTML, RUN, INCLUDE, DIRINCLUDE, EVAL, COMMENT, TRANSLATE, IF, ELIF, ELSE, ENDIF, CASE, WHEN, OTHERWISE, ENDCASE, WHILE, ENDWHILE, UNTIL, ENDUNTIL, FOR, ENDFOR, UNLESS, ELUN, UNELSE, ENDUNLESS, NOOP };
#else
enum tag_t { HTML, RUN, INCLUDE, DIRINCLUDE, EVAL, COMMENT, TRANSLATE, NOOP };
#endif


/* token structure */
typedef struct {
	script_t 	*script;	/* the parent script		*/
	enum tag_t	tag;		/* the token type		*/
	size_t		len;		/* length of token		*/
	char		*buf;		/* pointer to start of token	*/
	void		*next;		/* the next token in the chain	*/
	} token_t;



/* h_script.c */
script_t *load_script(char *filename, script_t *scriptlist);
void free_script_list(script_t *script);
token_t *push_token_on_list(token_t *tokenlist, script_t *scriptbuf, char *start, size_t len);
void free_token_list(token_t *tokenlist);

#ifndef JUST_LUACSHELL

token_t *build_token_list(script_t *scriptbuf, token_t *tokenlist);
void preprocess_token_list(token_t *tokenlist);
token_t *process_token_list(buffer_t *buf, token_t *tokenlist);

#endif

#endif /* !H_SCRIPT_H */
