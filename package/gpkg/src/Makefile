all: gpkg

gpkg: gpkg.o opkg_conf.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lericstools -lbbtargz -lewget

%.o: %.c
	$(CC) $(CFLAGS) -c  $^ -o $@ 
clean:
	rm -rf *.o *~ .*sw* gpkg
