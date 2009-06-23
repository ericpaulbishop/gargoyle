all: bw_set bw_get
bw_set: bw_set.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@  $^ -liptbwctl
bw_get: bw_get.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@  $^ -liptbwctl
clean:
	rm -rf bw_get bw_set *.o *~ .*sw*
