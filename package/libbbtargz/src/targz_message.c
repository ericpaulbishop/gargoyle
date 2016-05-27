
	
	struct errlist {
	char *errmsg;
	struct errlist *next;
};

static struct errlist *error_list_head, *error_list_tail;


	
#include <stdio.h>
#include "bbtargz.h"

	
static void
push_error_list(char *msg)
{
	struct errlist *e;

	e = xcalloc(1,  sizeof(struct errlist));
	e->errmsg = xstrdup(msg);
	e->next = NULL;

	if (error_list_head) {
		error_list_tail->next = e;
		error_list_tail = e;
	} else {
		error_list_head = error_list_tail = e;
	}
}

void
free_error_list(void)
{
	struct errlist *err, *err_tmp;

	err = error_list_head;
	while (err != NULL) {
		free(err->errmsg);
		err_tmp = err;
		err = err->next;
		free(err_tmp);
	}
}

void
print_error_list(void)
{
	struct errlist *err = error_list_head;

	if (err) {
		fprintf(stderr, "Collected errors:\n");
		/* Here we print the errors collected and free the list */
		while (err != NULL) {
			fprintf(stderr, " * %s", err->errmsg);
			err = err->next;
		}
	}
}


void
targz_message (message_level_t level, const char *fmt, ...)
{
	va_list ap;


	va_start (ap, fmt);

	if (level == ERROR) {
#define MSG_LEN 4096
		char msg[MSG_LEN];
		int ret;
		ret = vsnprintf(msg, MSG_LEN, fmt, ap);
		if (ret < 0) {
			fprintf(stderr, "%s: encountered an output or encoding"
					" error during vsnprintf.\n",
					__FUNCTION__);
			va_end (ap);
			exit(EXIT_FAILURE);
		}
		if (ret >= MSG_LEN) {
			fprintf(stderr, "%s: Message truncated.\n",
					__FUNCTION__);
		}
		push_error_list(msg);
	} else {
		if (vprintf(fmt, ap) < 0) {
			fprintf(stderr, "%s: encountered an output or encoding"
					" error during vprintf.\n",
					__FUNCTION__);
			exit(EXIT_FAILURE);
		}
	}

	va_end (ap);
}
