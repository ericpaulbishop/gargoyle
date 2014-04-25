/* --------------------------------------------------------------------------
 * core of haserl.cgi - a poor-man's php for embedded/lightweight environments
 * Copyright (c) 2003-2007   Nathan Angelacos (nangel@users.sourceforge.net)
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
 * -----
 * The x2c() and unescape_url() routines were taken from
 *  http://www.jmarshall.com/easy/cgi/getcgi.c.txt
 *
 * The comments in that text file state:
 *
 ***  Written in 1996 by James Marshall, james@jmarshall.com, except
 ***  that the x2c() and unescape_url() routines were lifted directly
 ***  from NCSA's sample program util.c, packaged with their HTTPD.
 ***     For the latest, see http://www.jmarshall.com/easy/cgi/
 * -----
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
#include <grp.h>

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "common.h"
#include <erics_tools.h>
#include "h_error.h"
#include "h_script.h"
#include "sliding_buffer.h"
#include "rfc2388.h"
#ifdef INCLUDE_BASHSHELL
#include "h_bash.h"
#endif
#include "h_translate.h"

#ifdef USE_LUA
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "h_lua_common.h"
#endif
#ifdef INCLUDE_LUASHELL
#include "h_lua.h"
#endif
#ifdef INCLUDE_LUACSHELL
#include "h_luac.h"
#endif

#include "haserl.h"

#ifndef TEMPDIR
#define TEMPDIR "/tmp"
#endif

#ifndef MAX_UPLOAD_KB
#define MAX_UPLOAD_KB 2048
#endif

/* Refuse to disable the subshell */
#ifndef SUBSHELL_CMD
#define SUBSHELL_CMD "/bin/sh"
#endif

haserl_t global;


/* declare the shell_ function pointers here */
void (*shell_exec) (buffer_t * buf, char *str);
void (*shell_echo) (buffer_t * buf, char *str, size_t len);
void (*shell_eval) (buffer_t * buf, char *str, size_t len);
void (*shell_setup) (char *, list_t *);
void (*shell_doscript) (buffer_t *, char *);
void (*shell_destroy) (void);

#ifdef BASHEXTENSIONS
void (*shell_if) (buffer_t * buf, char *str, size_t len);
void (*shell_elif) (buffer_t * buf, char *str, size_t len);
void (*shell_else) (buffer_t * buf, char *str, size_t len);
void (*shell_endif) (buffer_t * buf, char *str, size_t len);
void (*shell_case) (buffer_t * buf, char *str, size_t len);
void (*shell_when) (buffer_t * buf, char *str, size_t len);
void (*shell_otherwise) (buffer_t * buf, char *str, size_t len);
void (*shell_endcase) (buffer_t * buf, char *str, size_t len);
void (*shell_while) (buffer_t * buf, char *str, size_t len);
void (*shell_endwhile) (buffer_t * buf, char *str, size_t len);
void (*shell_until) (buffer_t * buf, char *str, size_t len);
void (*shell_enduntil) (buffer_t * buf, char *str, size_t len);
void (*shell_for) (buffer_t * buf, char *str, size_t len);
void (*shell_endfor) (buffer_t * buf, char *str, size_t len);
void (*shell_unless) (buffer_t * buf, char *str, size_t len);
void (*shell_elun) (buffer_t * buf, char *str, size_t len);
void (*shell_unelse) (buffer_t * buf, char *str, size_t len);
void (*shell_endunless) (buffer_t * buf, char *str, size_t len);
#endif

/* global shell execution function pointers.   These point to the actual functions
   that do the job, based on the language */

/*
 * Command line / Config file directives When adding a long option, make sure
 * to update the short_options as well
 */

struct option ga_long_options[] = {
  {"version", no_argument, 0, 'v'},
  {"help", no_argument, 0, 'h'},
  {"debug", no_argument, 0, 'd'},
  {"upload-limit", required_argument, 0, 'u'},
  {"upload-dir", required_argument, 0, 'U'},
  {"upload-handler", required_argument, 0, 'H'},
  {"accept-all", no_argument, 0, 'a'},
  {"accept-none", no_argument, 0, 'n'},
  {"shell", required_argument, 0, 's'},
  {"silent", no_argument, 0, 'S'},
  {0, 0, 0, 0}
};

const char *gs_short_options = "+vhdu:U:H:ans:S";

/*
 * Convert 2 char hex string into char it represents
 * (from http://www.jmarshall.com/easy/cgi)
 */
char
x2c (char *what)
{
  char digit;

  digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A') + 10 : (what[0] - '0'));
  digit *= 16;
  digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10 : (what[1] - '0'));
  return (digit);
}

/*
 * unsescape %xx to the characters they represent
 */
/* Modified by Juris Feb 2007 */
void
unescape_url (char *url)
{
  int i, j;
  for (i = 0, j = 0; url[j]; ++i, ++j)
    {
      if ((url[i] = url[j]) != '%')
	continue;
      if (!url[j + 1] || !url[j + 2])
	break;
      url[i] = x2c (&url[j + 1]);
      j += 2;
    }
  url[i] = '\0';
}




/*
 * allocate memory or die, busybox style.
 */
void *
xmalloc (size_t size)
{
  void *buf;
  if ((buf = malloc (size)) == NULL)
    {
      die_with_message (NULL, NULL, g_err_msg[E_MALLOC_FAIL]);
    }
  memset (buf, 0, size);
  return buf;
}


/*
 * realloc memory, or die xmalloc style.
 */
void *
xrealloc (void *buf, size_t size)
{
  if ((buf = realloc (buf, size)) == NULL)
    {
      die_with_message (NULL, NULL, g_err_msg[E_MALLOC_FAIL]);
    }
  return buf;
}


/*
 *   adds or replaces the "key=value" value in the env_list chain
 *   prefix is appended to the key (e.g. FORM_key=value)
 */
list_t *
myputenv (list_t * cur, char *str, char *prefix)
{
  list_t *prev = NULL;
  size_t keylen;
  char *entry = NULL;
  char *temp = NULL;
  int array = 0;
  int len;

  temp = memchr (str, '=', strlen (str));
  /* if we don't have an equal sign, exit early */
  if (temp == 0)
    {
      return (cur);
    }

  keylen = (size_t) (temp - str);

  /* is this an array */
  if (memcmp (str + keylen - 2, "[]", 2) == 0)
    {
      keylen = keylen - 2;
      array = 1;
    }

  entry = xmalloc (strlen (str) + strlen (prefix) + 1);
  entry[0] = '\0';
  if (strlen (prefix))
    {
      strncat (entry, prefix, strlen (prefix));
    }

  if (array == 1)
    {
      strncat (entry, str, keylen);
      strcat (entry, str + keylen + 2);
    }
  else
    {
      strcat (entry, str);
    }

  /* does the value already exist? */
  len = keylen + strlen (prefix) + 1;
  while (cur != NULL)
    {
      if (memcmp (cur->buf, entry, len) == 0)
	{
	  if (array == 1)
	    {
	      /* if an array, create a new string with this
	       * value added to the end of the old value(s) 
	       */
	      temp = xmalloc (strlen (cur->buf) + strlen(entry) - len + 1);
	      memmove (temp, cur->buf, strlen (cur->buf) + 1);
	      strcat (temp, "\n");
	      strcat (temp, str + keylen + 3);
	      free (entry);
	      entry = temp;
	    }
	  /* delete the old entry */
	  free (cur->buf);
	  if (prev != NULL)
	    prev->next = cur->next;
	  free (cur);
	  cur = prev;
	}			/* end if found a matching key */
      prev = cur;
      if (cur)
	{
	  cur = (list_t *) cur->next;
	}
    }				/* end if matching key */

  /* add the value to the end of the chain  */
  cur = xmalloc (sizeof (list_t));
  cur->buf = entry;
  if (prev != NULL)
    prev->next = cur;

  return (cur);
}

/* free list_t chain */
void
free_list_chain (list_t * list)
{
  list_t *next;

  while (list)
    {
      next = list->next;
      free (list->buf);
      free (list);
      list = next;
    }
}



/* readenv
 * reads the current environment and popluates our environment chain
 */

void
readenv (list_t * env)
{
  extern char **environ;
  int count = 0;

  while (environ[count] != NULL)
    {
      myputenv (env, environ[count], global.nul_prefix);
      count++;
    }
}


/* CookieVars ()
 * if HTTP_COOKIE is passed as an environment variable,
 * attempt to parse its values into environment variables
 */
void
CookieVars (list_t * env)
{
  char *qs;
  char *token;

  if (getenv ("HTTP_COOKIE") != NULL)
    {
      qs = strdup (getenv ("HTTP_COOKIE"));
    }
  else
    {
      return;
    }

	/** split on; to extract name value pairs */
  token = strtok (qs, ";");
  while (token)
    {
      // skip leading spaces
      while (token[0] == ' ')
	{
	  token++;
	}
      myputenv (env, token, global.var_prefix);
      myputenv (env, token, global.cookie_prefix);
      token = strtok (NULL, ";");
    }
  free (qs);
}


/* SessionID
 *  Makes a uniqe SESSIONID environment variable for this script
 */

void
sessionid (list_t * env)
{
  char session[29];

  sprintf (session, "SESSIONID=%x%x", getpid (), (int) time (NULL));
  myputenv (env, session, global.nul_prefix);
}

list_t *
wcversion (list_t * env)
{
  char version[200];
  sprintf (version, "HASERLVER=%s", PACKAGE_VERSION);
  return (myputenv (env, version, global.nul_prefix));
}


void
haserlflags (list_t * env)
{
  char buf[200];

  snprintf (buf, 200, "HASERL_UPLOAD_DIR=%s", global.uploaddir);
  myputenv (env, buf, global.nul_prefix);

  snprintf (buf, 200, "HASERL_UPLOAD_LIMIT=%lu", global.uploadkb);
  myputenv (env, buf, global.nul_prefix);

  snprintf (buf, 200, "HASERL_ACCEPT_ALL=%d", global.acceptall);
  myputenv (env, buf, global.nul_prefix);

  snprintf (buf, 200, "HASERL_SHELL=%s", global.shell);
  myputenv (env, buf, global.nul_prefix);

}

/*
 * Read cgi variables from query string, and put in environment
 */

int
ReadCGIQueryString (list_t * env)
{
  char *qs;
  char *token;
  int i;

  if (getenv ("QUERY_STRING") != NULL)
    {
      qs = strdup (getenv ("QUERY_STRING"));
    }
  else
    {
      return (0);
    }

  /* change plusses into spaces */
  for (i = 0; qs[i]; i++)
    {
      if (qs[i] == '+')
	{
	  qs[i] = ' ';
	}
    };

	/** split on & and ; to extract name value pairs */

  token = strtok (qs, "&;");
  while (token)
    {
      unescape_url (token);
      myputenv (env, token, global.var_prefix);
      myputenv (env, token, global.get_prefix);
      token = strtok (NULL, "&;");
    }
  free (qs);
  return (0);
}


/*
 * Read cgi variables from stdin (for POST queries)
 */

int
ReadCGIPOSTValues (list_t * env)
{
  size_t content_length = 0;
  size_t max_len;
  size_t i, j, x;
  sliding_buffer_t sbuf;
  buffer_t token;
  unsigned char *data;
  const char *CONTENT_LENGTH = "CONTENT_LENGTH";

  if ((getenv (CONTENT_LENGTH) == NULL) ||
      (strtoul (getenv (CONTENT_LENGTH), NULL, 10) == 0))
    return (0);

  if (getenv ("CONTENT_TYPE"))
    {
      if (strncasecmp (getenv ("CONTENT_TYPE"), "multipart/form-data", 19)
	  == 0)
	{
	  /* This is a mime request, we need to go to the mime handler */
	  i = rfc2388_handler (env);
	  return (i);
	}
    }

  s_buffer_init (&sbuf, 32768);
  sbuf.fh = STDIN;
  if (getenv (CONTENT_LENGTH))
    {
      sbuf.maxread = strtoul (getenv (CONTENT_LENGTH), NULL, 10);
    }
  haserl_buffer_init (&token);


  /* Allow 2MB content, unless they have a global upload set */
  max_len = ((global.uploadkb == 0) ? 2048 : global.uploadkb) *1024;

  do
    {
      /* x is true if this token ends with a matchstr or is at the end of stream */
      x = s_buffer_read (&sbuf, "&");
      content_length += sbuf.len;
      if (content_length > max_len)
	{
	  die_with_message (NULL, NULL,
			    "Attempted to send content larger than allowed limits.");
	}

      if ((x == 0) || (token.data))
	{
	  buffer_add (&token, (char *) sbuf.segment, sbuf.len);
	}

      if (x)
	{
	  data = sbuf.segment;
	  sbuf.segment[sbuf.len] = '\0';
	  if (token.data)
	    {
	      /* add the ASCIIZ */
	      buffer_add (&token, sbuf.segment + sbuf.len, 1);
	      data = token.data;
	    }

	  /* change plusses into spaces */
	  j = strlen ((char *) data);
	  for (i = 0; i <= j; i++)
	    {
	      if (data[i] == '+')
		{
		  data[i] = ' ';
		}
	    }
	  unescape_url ((char *) data);
	  myputenv (env, (char *) data, global.var_prefix);
	  myputenv (env, (char *) data, global.post_prefix);
	  if (token.data)
	    {
	      buffer_reset (&token);
	    }
	}
    }
  while (!sbuf.eof);
  s_buffer_destroy (&sbuf);
  buffer_destroy (&token);
  return (0);
}


int
parseCommandLine (int argc, char *argv[])
{
  int c;
  int option_index = 0;

  /* set optopt and optind to 0 to reset getopt_long -
   * we may call it multiple times
   */
  optopt = 0;
  optind = 0;

  while ((c = getopt_long (argc, argv, gs_short_options,
			   ga_long_options, &option_index)) != -1)
    {
      switch (c)
	{
	case 'd':
	  global.debug = TRUE;
	  break;
	case 's':
	  global.shell = optarg;
	  break;
	case 'S':
	  global.silent = TRUE;
	  break;
	case 'u':
	  if (optarg)
	    {
	      global.uploadkb = atoi (optarg);
	    }
	  else
	    {
	      global.uploadkb = MAX_UPLOAD_KB;
	    }
	  break;
	case 'a':
	  global.acceptall = TRUE;
	  break;
	case 'n':
	  global.acceptall = NONE;
	  break;
	case 'U':
	  global.uploaddir = optarg;
	  break;
	case 'H':
	  global.uploadhandler = optarg;
	  break;
	case 'v':
	case 'h':
	  printf ("This is " PACKAGE_NAME " version " PACKAGE_VERSION ""
		  " (http://haserl.sourceforge.net)\n");
	  exit (0);
	  break;
	}
    }
  return (optind);
}

int
BecomeUser (uid_t uid, gid_t gid)
{
  /* This silently fails if it doesn't work */
  /* Following is from Timo Teras */
  if (getuid () == 0)
    setgroups (1, &gid);

  setgid (gid);
  setgid (getgid ());

  setuid (uid);
  setuid (getuid ());

  return (0);
}

/*
 * Assign default values to the global structure
 */

void
assignGlobalStartupValues ()
{
  global.uploadkb = 0;		/* how big an upload do we allow (0 for none)   */
  global.shell = SUBSHELL_CMD;	/* The shell we use                             */
  global.silent = FALSE;	/* We do print errors if we find them           */
  global.uploaddir = TEMPDIR;	/* where to upload to                           */
  global.uploadhandler = NULL;	/* the upload handler                           */
  global.debug = FALSE;		/* Not in debug mode.                           */
  global.acceptall = FALSE;	/* don't allow POST data for GET method         */
  global.uploadlist = NULL;	/* we don't have any uploaded files             */
  global.var_prefix = "FORM_";
  global.get_prefix = "GET_";
  global.post_prefix = "POST_";
  global.cookie_prefix = "COOKIE_";
  global.nul_prefix = "";
  // translationKV_map gets filled only if a <%~ translation %> tag is found; and unknowable until preprocessing

  uci_init();

  global.webroot=uci_get("gargoyle", "global", "web_root");
  global.fallback_lang=uci_get("gargoyle", "global", "fallback_lang");
  if(global.fallback_lang == NULL)
  {
	  global.fallback_lang = strdup("English-EN");
  }
  global.active_lang=uci_get("gargoyle", "global", "language");
  if(global.active_lang == NULL)
  {
	  global.active_lang = strdup( global.fallback_lang );
  }
}


void
unlink_uploadlist ()
{
  token_t *me;
  me = global.uploadlist;

  while (me)
    {
      unlink (me->buf);
      free (me->buf);
      me = me->next;
    }

}



/*-------------------------------------------------------------------------
 *
 * Main
 *
 *------------------------------------------------------------------------*/

int
main (int argc, char *argv[])
{
#ifndef JUST_LUACSHELL
  token_t *tokenchain = NULL;
  buffer_t script_text;
#endif
  script_t *scriptchain;

  int retval = 0;
  char *filename = NULL;

  argv_t *av = NULL;
  char **av2 = argv;
  int av2c = argc;

  int command;
  int count;

  list_t *env = NULL;

  assignGlobalStartupValues ();
#ifndef JUST_LUACSHELL
  haserl_buffer_init (&script_text);
#endif

  /* if more than argv[1] and argv[1] is not a file */
  switch (argc)
    {
    case 1:
      /* we were run, instead of called as a shell script */
      puts ("This is " PACKAGE_NAME " version " PACKAGE_VERSION "\n"
	    "This program runs as a cgi interpeter, not interactively\n"
	    "Please see:  http://haserl.sourceforge.net\n"
#ifdef USE_LUA
	    "This version includes Lua (precompiled"
#ifdef INCLUDE_LUASHELL
	    " and interpreted"
#endif
	    ")\n"
#endif
#ifdef BASHEXTENSIONS
	    "Unsupported bash extensions supplied by simnux enabled\n"
#endif
	);
      return (0);
      break;
    default:			/* more than one */
      /* split combined #! args - linux bundles them as one */
      command = argc_argv (argv[1], &av, "");

      if (command > 1)
	{
	  /* rebuild argv into new av2 */
	  av2c = argc - 1 + command;
	  av2 = xmalloc (sizeof (char *) * av2c);
	  av2[0] = argv[0];
	  for (count = 1; count <= command; count++)
	    {
	      av2[count] = av[count - 1].string;
	    }
	  for (; count < av2c; count++)
	    {
	      av2[count] = argv[count - command + 1];
	    }
	}

      parseCommandLine (av2c, av2);
      free (av);
      if (av2 != argv)
	free (av2);

      if (optind < av2c)
	{
	  filename = av2[optind];
	}
      else
	{
	  die_with_message (NULL, NULL, "No script file specified");
	}

      break;
    }
    
    if (av2[0] != NULL && command == 1) { //prevents argument reconstruction (update.sh & reboot.sh)
		unsigned int prg_len=strlen(av2[0]);
		if (memcmp(av2[0]+prg_len-4, "i18n", 4) == 0) {
	
			if (global.translationKV_map == NULL) {
				buildTranslationMap ();
			}
			lookup_key (NULL, av2[optind], HASERL_SHELL_SYMBOLIC_LINK);
		
			buffer_destroy (&script_text);
		
			unsigned long num_destroyed;
			destroy_string_map(global.translationKV_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
		
			return (0);
		}
	}

  scriptchain = load_script (filename, NULL);
/* drop permissions */
  BecomeUser (scriptchain->uid, scriptchain->gid);

  /* populate the function pointers based on the shell selected */
  if (strcmp (global.shell, "lua") && strcmp (global.shell, "luac"))
    /* default to "bash" */
    {
#ifdef INCLUDE_BASHSHELL
      shell_exec = &bash_exec;
      shell_echo = &bash_echo;
      shell_eval = &bash_eval;
      shell_setup = &bash_setup;
      shell_doscript = &bash_doscript;
      shell_destroy = &bash_destroy;

#ifdef BASHEXTENSIONS
      shell_if = &bash_if;
      shell_elif = &bash_elif;
      shell_else = &bash_else;
      shell_endif = &bash_endif;
      shell_case = &bash_case;
      shell_when = &bash_when;
      shell_otherwise = &bash_otherwise;
      shell_endcase = &bash_endcase;
      shell_while = &bash_while;
      shell_endwhile = &bash_endwhile;
      shell_until = &bash_until;
      shell_enduntil = &bash_enduntil;
      shell_for = &bash_for;
      shell_endfor = &bash_endfor;
      shell_unless = &bash_unless;
      shell_elun = &bash_elun;
      shell_unelse = &bash_unelse;
      shell_endunless = &bash_endunless;
#endif

#else
      die_with_message (NULL, NULL, "Bash shell is not enabled.");
#endif
    }
  else
    {
#ifdef USE_LUA
      shell_setup = &lua_common_setup;
      shell_destroy = &lua_common_destroy;
      global.var_prefix = "FORM.";
      global.nul_prefix = "ENV.";

      if (global.shell[3] == 'c')	/* luac only */
#ifdef INCLUDE_LUACSHELL
	shell_doscript = &luac_doscript;
#else
	die_with_message (NULL, NULL, "Compiled Lua shell is not enabled.");
#endif
      else
	{
#ifdef INCLUDE_LUASHELL
	  shell_exec = &lua_exec;
	  shell_echo = &lua_echo;
	  shell_eval = &lua_eval;
	  shell_doscript = &lua_doscript;
#else
	  die_with_message (NULL, NULL, "Standard Lua shell is not enabled.");
#endif
	}
#else
      die_with_message (NULL, NULL, "Lua shells are not enabled.");
#endif
    }

/* Read the current environment into our chain */
  env = wcversion (env);
  readenv (env);
  sessionid (env);
  haserlflags (env);

#ifndef JUST_LUACSHELL
  if (strcmp (global.shell, "luac"))
    {
      tokenchain = build_token_list (scriptchain, NULL);
      preprocess_token_list (tokenchain);
    }
#endif

/* Read the request data */
  if (global.acceptall != NONE)
    {
      /* If we have a request method, and we were run as a #! style script */
      CookieVars (env);
      if (getenv ("REQUEST_METHOD"))
	{
	  if (strcasecmp (getenv ("REQUEST_METHOD"), "GET") == 0)
	    {
	      if (global.acceptall == TRUE)
		ReadCGIPOSTValues (env);
	      ReadCGIQueryString (env);
	    }

	  if (strcasecmp (getenv ("REQUEST_METHOD"), "POST") == 0)
	    {
	      if (global.acceptall == TRUE)
		retval = ReadCGIQueryString (env);
	      retval = ReadCGIPOSTValues (env);
	    }
	}
    }

/* build a copy of the script to send to the shell */
#ifndef JUST_LUACSHELL
  if (strcmp (global.shell, "luac"))
    {
      process_token_list (&script_text, tokenchain);
    }
#endif

  /* run the script */
  if (global.debug == TRUE)
    {
#ifndef JUST_LUACSHELL
      if (getenv ("REQUEST_METHOD"))
	{
	  write (1, "Content-Type: text/plain\n\n", 26);
	}
      write (1, script_text.data, script_text.ptr - script_text.data);
#else
      die_with_message (NULL, NULL,
			"Debugging output doesn't work with the compiled Lua shell.");
#endif
    }
  else
    {
      shell_setup (global.shell, env);
#ifdef JUST_LUACSHELL
      shell_doscript (NULL, scriptchain->name);
#else
      shell_doscript (&script_text, scriptchain->name);
#endif
      shell_destroy ();
    }
    //printf("%s\n", global.webroot);
    //printf("%s\n", global.fallback_lang);
    //printf("%s\n", global.active_lang);

  if (global.uploadlist)
    {
      unlink_uploadlist ();
      free_token_list (global.uploadlist);
    }

#ifndef JUST_LUACSHELL
  /* destroy the script */
  buffer_destroy (&script_text);
  free_token_list (tokenchain);
#endif

  free_list_chain (env);
  free_script_list (scriptchain);
    
  unsigned long num_destroyed;
  destroy_string_map(global.translationKV_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);

  return (0);

}
