all: sha1sum

sha1sum: sha1sum.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
clean:
	rm -rf *.o *~ .*sw* sha1sum
