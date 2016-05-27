/*
 * busybox library eXtended function
 *
 * Copyright (C) 2001 Larry Doolittle, <ldoolitt@recycle.lbl.gov>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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
 */

#include <string.h>
#include "bbtargz.h"

/* Find out if the last character of a string matches the one given Don't
 * underrun the buffer if the string length is 0.  Also avoids a possible
 * space-hogging inline of strlen() per usage.
 */
char * last_char_is(const char *s, int c)
{
	char *sret;
	if (!s)
	    return NULL;
	sret  = (char *)s+strlen(s)-1;
	if (sret>=s && *sret == c) {
		return sret;
	} else {
		return NULL;
	}
}
