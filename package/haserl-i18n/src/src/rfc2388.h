#ifndef _RFC2388_H
#define _RFC2388_H       1

typedef struct
{
  char  	*name;		/* the variable name */
  char  	*filename;	/* the client-specified filename */
  char  	*type;		/* the mime-type	*/
  char  	*tempname;	/* the tempfilename		*/
  buffer_t	value;		/* the value of the variable	*/
  int		fh;		/* the output file handle	*/
 } mime_var_t;


/* rfc2388.c */
void empty_stdin(void);
void mime_var_init(mime_var_t *obj);
void mime_var_destroy(mime_var_t *obj);
char *mime_substr(char *start, int len);
void mime_tag_add(mime_var_t *obj, char *str);
void mime_var_putenv(list_t *env, mime_var_t *obj);
void mime_exec(mime_var_t *obj, char *fifo);
void mime_var_open_target(mime_var_t *obj);
void mime_var_writer(mime_var_t *obj, char *str, int len);
int rfc2388_handler(list_t *env);


#endif /* _RFC2388_H */
