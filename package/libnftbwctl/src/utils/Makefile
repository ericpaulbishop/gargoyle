all: bw_set bw_get bw_print_history_file set_kernel_timezone
bw_set: bw_set.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@  $^ -lnftbwctl
bw_get: bw_get.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@  $^ -lnftbwctl
bw_print_history_file: bw_print_history_file.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@  $^ -lnftbwctl
set_kernel_timezone: set_kernel_timezone.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@  $^ -lnftbwctl

clean:
	rm -rf bw_set bw_get print_history_file set_kernel_timezone *.o *~ .*sw*
