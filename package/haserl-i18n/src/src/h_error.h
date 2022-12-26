/*
 * $Id: haserl.h,v 1.14 2005/11/18 14:43:10 nangel Exp $
 */
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
