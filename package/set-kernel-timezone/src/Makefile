all: set_kernel_timezone
set_kernel_timezone: set_kernel_timezone.c
	$(CC) $(CFLAGS) $(LDFLAGS) set_kernel_timezone.c -o set_kernel_timezone
clean:
	rm -rf set_kernel_timezone *~ .*sw*
