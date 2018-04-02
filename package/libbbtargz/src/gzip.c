/*
 *  Copyright (C) 2016 Jo-Philipp Wich <jo@mein.io>
 *  Copyright (C) 2016 Felix Fietkau <nbd@nbd.name>
 *
 *  Zlib decrompression utility routines.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "bbtargz.h"

static void to_devnull(int fd)
{
	int devnull = open("/dev/null", fd ? O_WRONLY : O_RDONLY);

	if (devnull >= 0)
		dup2(devnull, fd);

	if (devnull > STDERR_FILENO)
		close(devnull);
}

static void *gzip_thread(void *ptr)
{
	struct gzip_handle *zh = ptr;
	char buf[4096];
	int len = 0, ret;

	while (1) {
		if (zh->file)
			len = fread(buf, 1, sizeof(buf), zh->file);
		else if (zh->gzip)
			len = gzip_read(zh->gzip, buf, sizeof(buf));

		if (len <= 0)
			break;

		do {
			ret = write(zh->wfd, buf, len);
		} while (ret == -1 && errno == EINTR);
	}

	close(zh->wfd);
	zh->wfd = -1;

	return NULL;
}

int gzip_exec(struct gzip_handle *zh, const char *filename)
{
	int rpipe[2] = { -1, -1 }, wpipe[2] = {
	-1, -1};
	struct sigaction pipe_sa = {.sa_handler = SIG_IGN };

	zh->rfd = -1;
	zh->wfd = -1;

	if (sigaction(SIGPIPE, &pipe_sa, &zh->pipe_sa) < 0)
		return -1;

	if (pipe(rpipe) < 0)
		return -1;

	if (!filename && pipe(wpipe) < 0) {
		close(rpipe[0]);
		close(rpipe[1]);
		return -1;
	}

	zh->pid = vfork();

	switch (zh->pid) {
	case -1:
		return -1;

	case 0:
		to_devnull(STDERR_FILENO);

		if (filename) {
			to_devnull(STDIN_FILENO);
		} else {
			dup2(wpipe[0], STDIN_FILENO);
			close(wpipe[0]);
			close(wpipe[1]);
		}

		dup2(rpipe[1], STDOUT_FILENO);
		close(rpipe[0]);
		close(rpipe[1]);

		execlp("gzip", "gzip", "-d", "-c", filename, NULL);
		exit(-1);

	default:
		zh->rfd = rpipe[0];
		zh->wfd = wpipe[1];

		fcntl(zh->rfd, F_SETFD, fcntl(zh->rfd, F_GETFD) | FD_CLOEXEC);
		close(rpipe[1]);

		if (zh->wfd >= 0) {
			fcntl(zh->wfd, F_SETFD,
			      fcntl(zh->wfd, F_GETFD) | FD_CLOEXEC);
			close(wpipe[0]);
			pthread_create(&zh->thread, NULL, gzip_thread, zh);
		}
	}

	return 0;
}

ssize_t gzip_read(struct gzip_handle * zh, void *buf, ssize_t len)
{
	ssize_t ret;

	do {
		ret = read(zh->rfd, buf, len);
	} while (ret == -1 && errno != EINTR);

	return ret;
}

ssize_t gzip_copy(struct gzip_handle * zh, FILE * out, ssize_t len)
{
	char buf[4096];
	ssize_t rlen, total = 0;

	while (len > 0) {
		rlen = gzip_read(zh, buf,
				 (len > sizeof(buf)) ? sizeof(buf) : len);

		if (rlen <= 0)
			break;

		if (out != NULL) {
			if (fwrite(buf, 1, rlen, out) != rlen)
				break;
		}

		len -= rlen;
		total += rlen;
	}

	return total;
}

FILE *gzip_fdopen(struct gzip_handle * zh, const char *filename)
{
	memset(zh, 0, sizeof(*zh));

	if (!filename || gzip_exec(zh, filename) < 0)
		return NULL;

	fcntl(zh->rfd, F_SETFL, fcntl(zh->rfd, F_GETFL) & ~O_NONBLOCK);

	return fdopen(zh->rfd, "r");
}

int gzip_close(struct gzip_handle *zh)
{
	int code = -1;

	if (zh->rfd >= 0)
		close(zh->rfd);

	if (zh->wfd >= 0)
		close(zh->wfd);

	if (zh->pid > 0) {
		kill(zh->pid, SIGKILL);
		waitpid(zh->pid, &code, 0);
	}

	if (zh->file)
		fclose(zh->file);

	if (zh->thread)
		pthread_join(zh->thread, NULL);

	sigaction(SIGPIPE, &zh->pipe_sa, NULL);

	return WIFEXITED(code) ? WEXITSTATUS(code) : -1;
}
