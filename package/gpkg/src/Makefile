all: gpkg

gpkg: gpkg.o conf.o load.o xsystem.o update.o remove.o install.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lericstools -lbbtargz -lewget

%.o: %.c
	$(CC) $(CFLAGS) -c  $^ -o $@ 
clean:
	rm -rf *.o *~ .*sw* gpkg
