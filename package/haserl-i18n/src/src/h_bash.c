/*-----------------------------------------------------------------
 * haserl functions specific to a bash/ash/dash shell
 * Copyright (c) 2003-2007    Nathan Angelacos (nangel@users.sourceforge.net)
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by the Free
 * Software Foundation.
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
 ------------------------------------------------------------------------- */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "common.h"
#include <erics_tools.h>
#include "h_error.h"
#include "h_bash.h"
#include "h_script.h"
#include "haserl.h"

/* Local subshell variables */
static int subshell_pipe[2];
static int subshell_pid;


void
bash_setup (char *shell, list_t * env)
{
  int retcode = 0;
  int count;
  argv_t *argv;
  char *av[20];
  list_t *next;

  if (shell == NULL)
    return;

  retcode = pipe (&subshell_pipe[PARENT_IN]);
  if (retcode == 0)
    {
      subshell_pid = fork ();
      if (subshell_pid == -1)
	{
	  die_with_message (NULL, NULL, g_err_msg[E_SUBSHELL_FAIL]);
	}

      if (subshell_pid == 0)
	{
	  /* I'm the child, connect stdin to the parent */
	  dup2 (subshell_pipe[PARENT_IN], STDIN_FILENO);
	  close (subshell_pipe[PARENT_IN]);
	  close (subshell_pipe[PARENT_OUT]);
	  count = argc_argv (shell, &argv, "");
	  if (count > 19)
	    {
	      /* over 20 command line args, silently truncate */
	      av[19] = "\0";
	      count = 18;
	    }
	  while (count >= 0)
	    {
	      av[count] = argv[count].string;
	      count--;
	    }


	  /* populate the environment */
	  while (env)
	    {
	      next = env->next;
	      putenv (env->buf);
	      env = next;
	    }

	  execv (argv[0].string, av);
	  free (argv);

	  /* if we get here, we had a failure */
	  die_with_message (NULL, NULL, g_err_msg[E_SUBSHELL_FAIL]);
	}
      else
	{
	  /* I'm parent, move along please */
	  close (subshell_pipe[PARENT_IN]);
	}
    }

  /* control should get to this point only in the parent.
   */
}

void
bash_destroy (void)
{
  int status;
  waitpid (subshell_pid, &status, 0);
}


void
bash_exec (buffer_t * buf, char *str)
{
  buffer_add (buf, str, strlen (str));
  return;
}

/* Run the echo command in a subshell */
void
bash_echo (buffer_t * buf, char *str, size_t len)
{
/* limits.h would tell us the ARG_MAX characters we COULD send to the echo command, but
 * we will take the (ancient) POSIX1 standard of 4K, subtract 1K from it and use that
 * as the maxmimum.    The Linux limit appears to be 128K, so 3K will fit. */

  static char echo_start[] = "echo -n '";
  static char echo_quote[] = "'\\''";
  static char echo_end[] = "'\n";
  const size_t maxlen = 3096;
  size_t pos;

  if (len == 0)
    return;
  pos = 0;

  buffer_add (buf, echo_start, strlen (echo_start));
  while (pos < len)
    {
      if (str[pos] == '\'')
	buffer_add (buf, echo_quote, strlen (echo_quote));
      else
	buffer_add (buf, str + pos, 1);
      pos++;
      if ((pos % maxlen) == 0)
	{
	  buffer_add (buf, echo_end, strlen (echo_end));
	  buffer_add (buf, echo_start, strlen (echo_start));
	}
    }
  buffer_add (buf, echo_end, strlen (echo_end));
}


/* do an evaluation in a subshell */
void
bash_eval (buffer_t * buf, char *str, size_t len)
{
  static char echo_start[] = "echo -n ";
  static char echo_end[] = "\n";
  if (len == 0)
    return;

  buffer_add (buf, echo_start, strlen (echo_start));
  buffer_add (buf, str, len);
  buffer_add (buf, echo_end, strlen (echo_end));
}

#ifdef BASHEXTENSIONS
/* generate an IF statment */
void
bash_if (buffer_t * buf, char *str, size_t len)
{
  static char err_msg[] =
    "echo 'error: missing expression for if'\nexit 99\n";
  static char if_start[] = "if [[ ";
  static char if_end[] = " ]]\nthen\n";
  static char ex_start[] = "if ";
  static char ex_end[] = "\nthen\n";

  if (len == 0)
    {
      buffer_add (buf, err_msg, strlen (err_msg));
    }
  else if (str[0] == '|')
    {
      str[0] = ' ';
      buffer_add (buf, ex_start, strlen (ex_start));
      buffer_add (buf, str, len);
      buffer_add (buf, ex_end, strlen (ex_end));
    }
  else
    {
      buffer_add (buf, if_start, strlen (if_start));
      buffer_add (buf, str, len);
      buffer_add (buf, if_end, strlen (if_end));
    }
}


/* generate an ELIF statment */
void
bash_elif (buffer_t * buf, char *str, size_t len)
{
  static char err_msg[] =
    "echo 'error: missing expression for elif'\nexit 99\n";
  static char elif_start[] = "elif [[ ";
  static char elif_end[] = " ]]\nthen\n";
  static char ex_start[] = "elif ";
  static char ex_end[] = "\nthen\n";

  if (len == 0)
    {
      buffer_add (buf, err_msg, strlen (err_msg));
    }
  else if (str[0] == '|')
    {
      str[0] = ' ';
      buffer_add (buf, ex_start, strlen (ex_start));
      buffer_add (buf, str, len);
      buffer_add (buf, ex_end, strlen (ex_end));
    }
  else
    {
      buffer_add (buf, elif_start, strlen (elif_start));
      buffer_add (buf, str, len);
      buffer_add (buf, elif_end, strlen (elif_end));
    }
}


/* generate an ELSE statment */
void
bash_else (buffer_t * buf, char *str, size_t len)
{
  static char else_start[] = "else";
  static char else_start2[] = "else #";
  static char else_end[] = "\n";

  if (len == 0)
    {
      buffer_add (buf, else_start, strlen (else_start));
    }
  else
    {
      buffer_add (buf, else_start2, strlen (else_start2));
      buffer_add (buf, str, len);
    }
  buffer_add (buf, else_end, strlen (else_end));
}


/* generate a FI statment */
void
bash_endif (buffer_t * buf, char *str, size_t len)
{
  static char fi_start[] = "fi";
  static char fi_start2[] = "fi #";
  static char fi_end[] = "\n";

  if (len == 0)
    {
      buffer_add (buf, fi_start, strlen (fi_start));
    }
  else
    {
      buffer_add (buf, fi_start2, strlen (fi_start2));
      buffer_add (buf, str, len);
    }
  buffer_add (buf, fi_end, strlen (fi_end));
}


/* generate a CASE statment */
void
bash_case (buffer_t * buf, char *str, size_t len)
{
  static char err_msg[] =
    "echo 'error: missing expression for case'\nexit 99\n";
  static char case_start[] = "case ";
  static char case_end[] = " in\n";
  /*
     create a bogus case condition, nul+esc+eof, so nl+;;+nl
     can be prepended to each when/otherwise/endcase, which
     eliminates the need for ;; or <% ;; %> in the page source
   */
  static char case_bogus[] = "\"\\000\\040\\004\") :\n";

  if (len == 0)
    {
      buffer_add (buf, err_msg, strlen (err_msg));
    }
  else
    {
      buffer_add (buf, case_start, strlen (case_start));
      buffer_add (buf, str, len);
      buffer_add (buf, case_end, strlen (case_end));
      buffer_add (buf, case_bogus, strlen (case_bogus));
    }
}


/* generate a WHEN statment */
void
bash_when (buffer_t * buf, char *str, size_t len)
{
  static char err_msg[] =
    "echo 'error: missing expression for when'\nexit 99\n";
  static char when_start[] = "\n;;\n";
  static char when_end[] = ")\n";

  if (len == 0)
    {
      buffer_add (buf, err_msg, strlen (err_msg));
    }
  else
    {
      buffer_add (buf, when_start, strlen (when_start));
      buffer_add (buf, str, len - 1);
      buffer_add (buf, when_end, strlen (when_end));
    }
}


/* generate an OTHERWISE statment */
void
bash_otherwise (buffer_t * buf, char *str, size_t len)
{
  static char otherwise_start[] = "\n;;\n";
  static char otherwise_start1[] = "*)";
  static char otherwise_start2[] = "*) #";
  static char otherwise_end[] = "\n";

  buffer_add (buf, otherwise_start, strlen (otherwise_start));

  if (len == 0)
    {
      buffer_add (buf, otherwise_start1, strlen (otherwise_start1));
    }
  else
    {
      buffer_add (buf, otherwise_start2, strlen (otherwise_start2));
      buffer_add (buf, str, len);
    }
  buffer_add (buf, otherwise_end, strlen (otherwise_end));
}


/* generate a ENDCASE statment */
void
bash_endcase (buffer_t * buf, char *str, size_t len)
{
  static char endcase_start[] = "\n;;\n";
  static char endcase_start1[] = "esac";
  static char endcase_start2[] = "esac #";
  static char endcase_end[] = "\n";

  buffer_add (buf, endcase_start, strlen (endcase_start));

  if (len == 0)
    {
      buffer_add (buf, endcase_start1, strlen (endcase_start1));
    }
  else
    {
      buffer_add (buf, endcase_start2, strlen (endcase_start2));
      buffer_add (buf, str, len);
    }
  buffer_add (buf, endcase_end, strlen (endcase_end));
}


/* generate a WHILE statment */
void
bash_while (buffer_t * buf, char *str, size_t len)
{
  static char err_msg[] =
    "echo 'error: missing expression for while'\nexit 99\n";
  static char while_start[] = "while [[ ";
  static char while_end[] = " ]]\ndo\n";
  static char ex_start[] = "while ";
  static char ex_end[] = "\ndo\n";

  if (len == 0)
    {
      buffer_add (buf, err_msg, strlen (err_msg));
    }
  else if (str[0] == '|')
    {
      str[0] = ' ';
      buffer_add (buf, ex_start, strlen (ex_start));
      buffer_add (buf, str, len);
      buffer_add (buf, ex_end, strlen (ex_end));
    }
  else
    {
      buffer_add (buf, while_start, strlen (while_start));
      buffer_add (buf, str, len);
      buffer_add (buf, while_end, strlen (while_end));
    }
}


/* generate an ENDWHILE statment */
void
bash_endwhile (buffer_t * buf, char *str, size_t len)
{
  static char endwhile_start[] = "done";
  static char endwhile_start2[] = "done #";
  static char endwhile_end[] = "\n";

  if (len == 0)
    {
      buffer_add (buf, endwhile_start, strlen (endwhile_start));
    }
  else
    {
      buffer_add (buf, endwhile_start2, strlen (endwhile_start2));
      buffer_add (buf, str, len);
    }
  buffer_add (buf, endwhile_end, strlen (endwhile_end));
}


/* generate an UNTIL statment */
void
bash_until (buffer_t * buf, char *str, size_t len)
{
  static char err_msg[] =
    "echo 'error: missing expression for until'\nexit 99\n";
  static char until_start[] = "until [[ ";
  static char until_end[] = " ]]\ndo\n";
  static char ex_start[] = "until ";
  static char ex_end[] = "\ndo\n";

  if (len == 0)
    {
      buffer_add (buf, err_msg, strlen (err_msg));
    }
  else if (str[0] == '|')
    {
      str[0] = ' ';
      buffer_add (buf, ex_start, strlen (ex_start));
      buffer_add (buf, str, len);
      buffer_add (buf, ex_end, strlen (ex_end));
    }
  else
    {
      buffer_add (buf, until_start, strlen (until_start));
      buffer_add (buf, str, len);
      buffer_add (buf, until_end, strlen (until_end));
    }
}


/* generate an ENDUNTIL statment */
void
bash_enduntil (buffer_t * buf, char *str, size_t len)
{
  static char enduntil_start[] = "done";
  static char enduntil_start2[] = "done #";
  static char enduntil_end[] = "\n";

  if (len == 0)
    {
      buffer_add (buf, enduntil_start, strlen (enduntil_start));
    }
  else
    {
      buffer_add (buf, enduntil_start2, strlen (enduntil_start2));
      buffer_add (buf, str, len);
    }
  buffer_add (buf, enduntil_end, strlen (enduntil_end));
}


/* generate a FOR statment */
void
bash_for (buffer_t * buf, char *str, size_t len)
{
  static char err_msg[] =
    "echo 'error: missing expression for for'\nexit 99\n";
  static char for_start[] = "for ";
  static char for_end[] = "\ndo\n";

  if (len == 0)
    {
      buffer_add (buf, err_msg, strlen (err_msg));
    }
  else
    {
      buffer_add (buf, for_start, strlen (for_start));
      buffer_add (buf, str, len);
      buffer_add (buf, for_end, strlen (for_end));
    }
}


/* generate an ENDFOR statment */
void
bash_endfor (buffer_t * buf, char *str, size_t len)
{
  static char endfor_start[] = "done";
  static char endfor_start2[] = "done #";
  static char endfor_end[] = "\n";

  if (len == 0)
    {
      buffer_add (buf, endfor_start, strlen (endfor_start));
    }
  else
    {
      buffer_add (buf, endfor_start2, strlen (endfor_start2));
      buffer_add (buf, str, len);
    }
  buffer_add (buf, endfor_end, strlen (endfor_end));
}


/* generate an UNLESS statment */
void
bash_unless (buffer_t * buf, char *str, size_t len)
{
  static char err_msg[] =
    "echo 'error: missing expression for unless'\nexit 99\n";
  static char unless_start[] = "if [[ ! ( ";
  static char unless_end[] = " ) ]]\nthen\n";
  static char ex_start[] = "if ! ";
  static char ex_end[] = "\nthen\n";

  if (len == 0)
    {
      buffer_add (buf, err_msg, strlen (err_msg));
    }
  else if (str[0] == '|')
    {
      str[0] = ' ';
      buffer_add (buf, ex_start, strlen (ex_start));
      buffer_add (buf, str, len);
      buffer_add (buf, ex_end, strlen (ex_end));
    }
  else
    {
      buffer_add (buf, unless_start, strlen (unless_start));
      buffer_add (buf, str, len);
      buffer_add (buf, unless_end, strlen (unless_end));
    }
}


/* generate an ELUN statment */
void
bash_elun (buffer_t * buf, char *str, size_t len)
{
  static char err_msg[] =
    "echo 'error: missing expression for elun'\nexit 99\n";
  static char elun_start[] = "elif [[ ! ( ";
  static char elun_end[] = " ) ]]\nthen\n";
  static char ex_start[] = "elif ! ";
  static char ex_end[] = "\nthen\n";

  if (len == 0)
    {
      buffer_add (buf, err_msg, strlen (err_msg));
    }
  else if (str[0] == '|')
    {
      str[0] = ' ';
      buffer_add (buf, ex_start, strlen (ex_start));
      buffer_add (buf, str, len);
      buffer_add (buf, ex_end, strlen (ex_end));
    }
  else
    {
      buffer_add (buf, elun_start, strlen (elun_start));
      buffer_add (buf, str, len);
      buffer_add (buf, elun_end, strlen (elun_end));
    }
}


/* generate an UNELSE statment */
void
bash_unelse (buffer_t * buf, char *str, size_t len)
{
  static char unelse_start[] = "else";
  static char unelse_start2[] = "else #";
  static char unelse_end[] = "\n";

  if (len == 0)
    {
      buffer_add (buf, unelse_start, strlen (unelse_start));
    }
  else
    {
      buffer_add (buf, unelse_start2, strlen (unelse_start2));
      buffer_add (buf, str, len);
    }
  buffer_add (buf, unelse_end, strlen (unelse_end));
}


/* generate a ENDUNLESS statment */
void
bash_endunless (buffer_t * buf, char *str, size_t len)
{
  static char endunless_start[] = "fi";
  static char endunless_start2[] = "fi #";
  static char endunless_end[] = "\n";

  if (len == 0)
    {
      buffer_add (buf, endunless_start, strlen (endunless_start));
    }
  else
    {
      buffer_add (buf, endunless_start2, strlen (endunless_start2));
      buffer_add (buf, str, len);
    }
  buffer_add (buf, endunless_end, strlen (endunless_end));
}
#endif

void
bash_doscript (buffer_t * script, char *name)
{
  static char postfix[] = "\nexit\n";

  /* dump the script to the subshell */
  write (subshell_pipe[PARENT_OUT], script->data, script->ptr - script->data);

  /* write the postfix */
  write (subshell_pipe[PARENT_OUT], postfix, strlen (postfix));


  return;

}
