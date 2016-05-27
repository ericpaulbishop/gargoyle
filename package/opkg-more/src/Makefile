all: opkg-more

opkg-more: opkg-more.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lericstools -lbbtargz

opkg-more.o: opkg-more.c
	$(CC) $(CFLAGS) -c  $^ -o $@ 
clean:
	rm -rf *.o *~ .*sw* opkg-more
