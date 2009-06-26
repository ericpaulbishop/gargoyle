all: bw_set bw_get set_kernel_timezone
set_kernel_timezone: set_kernel_timezone.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@  $^ -liptbwctl
bw_set: bw_set.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@  $^ -liptbwctl
bw_get: bw_get.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@  $^ -liptbwctl
clean:
	rm -rf bw_get bw_set set_kernel_timezone *.o *~ .*sw*
