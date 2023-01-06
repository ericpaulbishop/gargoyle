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

#ifndef _COMMON_H
#define _COMMON_H	1


/* how many argv slots to allocate at once */
#define ALLOC_CHUNK	10


#define STDIN	0
#define STDOUT  1
#define STDERR	2


#define TRUE  -1
#define FALSE 0
#define NONE  1

/* linked list */
typedef struct
{
  char *buf;
  void *next;
} list_t;

/* name/value pairs */
typedef struct
{
  char *string;			/* the string */
  unsigned char quoted;		/* non-zero if the string was quoted */
} argv_t;

/* expandable buffer structure */
typedef struct
{
  unsigned char *data;		/* the data */
  unsigned char *ptr;		/* where to write to next */
  unsigned char *limit;		/* maximal allocated buffer pos */
} buffer_t;


/* common.c */

int argc_argv (char *instr, argv_t ** argv, char *commentstr);
void haserl_buffer_init (buffer_t * buf);
void buffer_reset (buffer_t * buf);
void buffer_destroy (buffer_t * buf);
void buffer_add (buffer_t * buf, const void *data, unsigned long size);

#ifndef JUST_LUACSHELL

void uppercase (char *instr);
void lowercase (char *instr);
char *skip_whitespace (char *instr);
char *find_whitespace (char *instr);
int count_lines (char *instr, size_t len, char *where);

#endif

#endif /* !_COMMON_H */
