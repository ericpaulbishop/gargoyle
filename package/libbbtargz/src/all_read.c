/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) 1999-2004 by Erik Andersen <andersen@codepoet.org>
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
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "bbtargz.h"

extern void archive_xread_all(int fd , char *buf, size_t count)
{
        ssize_t size;

        size = full_read(fd, buf, count);
        if (size != count) {
                perror_msg_and_die("Short read");
        }
        return;
}

/*
 * Read all of the supplied buffer from a file.
 * This does multiple reads as necessary.
 * Returns the amount read, or -1 on an error.
 * A short read is returned on an end of file.
 */
ssize_t full_read(int fd, char *buf, int len)
{
        ssize_t cc;
        ssize_t total;

        total = 0;

        while (len > 0) {
                cc = safe_read(fd, buf, len);

                if (cc < 0)
                        return cc;      /* read() returns -1 on failure. */

                if (cc == 0)
                        break;

                buf = ((char *)buf) + cc;
                total += cc;
                len -= cc;
        }

        return total;
}


ssize_t safe_read(int fd, void *buf, size_t count)
{
        ssize_t n;

        do {
                n = read(fd, buf, count);
        } while (n < 0 && errno == EINTR);

        return n;
}



/* END CODE */

