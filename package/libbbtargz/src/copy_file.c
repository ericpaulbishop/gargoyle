/* vi: set sw=4 ts=4: */
/*
 * Mini copy_file implementation for busybox
 *
 *
 * Copyright (C) 2001 by Matt Kraai <kraai@alumni.carnegiemellon.edu>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <utime.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include "bbtargz.h"

int copy_file(const char *source, const char *dest, int flags)
{
	struct stat source_stat;
	struct stat dest_stat;
	int dest_exists = 1;
	int status = 0;

	if (((flags & FILEUTILS_PRESERVE_SYMLINKS) &&
			lstat(source, &source_stat) < 0) ||
			(!(flags & FILEUTILS_PRESERVE_SYMLINKS) &&
			 stat(source, &source_stat) < 0)) {
		perror_msg("%s", source);
		return -1;
	}

	if (stat(dest, &dest_stat) < 0) {
		if (errno != ENOENT) {
			perror_msg("unable to stat `%s'", dest);
			return -1;
		}
		dest_exists = 0;
	}

	if (dest_exists && source_stat.st_rdev == dest_stat.st_rdev &&
			source_stat.st_ino == dest_stat.st_ino) {
		error_msg("`%s' and `%s' are the same file", source, dest);
		return -1;
	}

	if (S_ISDIR(source_stat.st_mode)) {
		DIR *dp;
		struct dirent *d;
		mode_t saved_umask = 0;

		if (!(flags & FILEUTILS_RECUR)) {
			error_msg("%s: omitting directory", source);
			return -1;
		}

		/* Create DEST.  */
		if (dest_exists) {
			if (!S_ISDIR(dest_stat.st_mode)) {
				error_msg("`%s' is not a directory", dest);
				return -1;
			}
		} else {
			mode_t mode;
			saved_umask = umask(0);

			mode = source_stat.st_mode;
			if (!(flags & FILEUTILS_PRESERVE_STATUS))
				mode = source_stat.st_mode & ~saved_umask;
			mode |= S_IRWXU;

			if (mkdir(dest, mode) < 0) {
				umask(saved_umask);
				perror_msg("cannot create directory `%s'", dest);
				return -1;
			}

			umask(saved_umask);
		}

		/* Recursively copy files in SOURCE.  */
		if ((dp = opendir(source)) == NULL) {
			perror_msg("unable to open directory `%s'", source);
			status = -1;
			goto end;
		}

		while ((d = readdir(dp)) != NULL) {
			char *new_source, *new_dest;

			if (strcmp(d->d_name, ".") == 0 ||
					strcmp(d->d_name, "..") == 0)
				continue;

			new_source = concat_path_file(source, d->d_name);
			new_dest = concat_path_file(dest, d->d_name);
			if (copy_file(new_source, new_dest, flags) < 0)
				status = -1;
			free(new_source);
			free(new_dest);
		}

		/* ??? What if an error occurs in readdir?  */

		if (closedir(dp) < 0) {
			perror_msg("unable to close directory `%s'", source);
			status = -1;
		}

		if (!dest_exists &&
				chmod(dest, source_stat.st_mode & ~saved_umask) < 0) {
			perror_msg("unable to change permissions of `%s'", dest);
			status = -1;
		}
	} else if (S_ISREG(source_stat.st_mode)) {
		FILE *sfp, *dfp;

		if (dest_exists) {
			if ((dfp = fopen(dest, "w")) == NULL) {
				if (!(flags & FILEUTILS_FORCE)) {
					perror_msg("unable to open `%s'", dest);
					return -1;
				}

				if (unlink(dest) < 0) {
					perror_msg("unable to remove `%s'", dest);
					return -1;
				}

				dest_exists = 0;
			}
		}

		if (!dest_exists) {
			int fd;

			if ((fd = open(dest, O_WRONLY|O_CREAT, source_stat.st_mode)) < 0 ||
					(dfp = fdopen(fd, "w")) == NULL) {
				if (fd >= 0)
					close(fd);
				perror_msg("unable to open `%s'", dest);
				return -1;
			}
		}

		if ((sfp = fopen(source, "r")) == NULL) {
			fclose(dfp);
			perror_msg("unable to open `%s'", source);
			status = -1;
			goto end;
		}

		if (copy_file_chunk(sfp, dfp, -1) < 0)
			status = -1;

		if (fclose(dfp) < 0) {
			perror_msg("unable to close `%s'", dest);
			status = -1;
		}

		if (fclose(sfp) < 0) {
			perror_msg("unable to close `%s'", source);
			status = -1;
		}
	} else if (S_ISBLK(source_stat.st_mode) || S_ISCHR(source_stat.st_mode) ||
			S_ISSOCK(source_stat.st_mode)) {
		if (mknod(dest, source_stat.st_mode, source_stat.st_rdev) < 0) {
			perror_msg("unable to create `%s'", dest);
			return -1;
		}
	} else if (S_ISFIFO(source_stat.st_mode)) {
		if (mkfifo(dest, source_stat.st_mode) < 0) {
			perror_msg("cannot create fifo `%s'", dest);
			return -1;
		}
	} else if (S_ISLNK(source_stat.st_mode)) {
		char *lpath = xreadlink(source);
		if (symlink(lpath, dest) < 0) {
			perror_msg("cannot create symlink `%s'", dest);
			return -1;
		}
		free(lpath);

#if (__GLIBC__ >= 2) && (__GLIBC_MINOR__ >= 1)
		if (flags & FILEUTILS_PRESERVE_STATUS)
			if (lchown(dest, source_stat.st_uid, source_stat.st_gid) < 0)
				perror_msg("unable to preserve ownership of `%s'", dest);
#endif
		return 0;
	} else {
		error_msg("internal error: unrecognized file type");
		return -1;
	}

end:

	if (flags & FILEUTILS_PRESERVE_STATUS) {
		struct utimbuf times;

		times.actime = source_stat.st_atime;
		times.modtime = source_stat.st_mtime;
		if (utime(dest, &times) < 0)
			perror_msg("unable to preserve times of `%s'", dest);
		if (chown(dest, source_stat.st_uid, source_stat.st_gid) < 0) {
			source_stat.st_mode &= ~(S_ISUID | S_ISGID);
			perror_msg("unable to preserve ownership of `%s'", dest);
		}
		if (chmod(dest, source_stat.st_mode) < 0)
			perror_msg("unable to preserve permissions of `%s'", dest);
	}

	return status;
}
