LIBS:= -lericstools
CFLAGS:= -Wall
LIBIPTC:= /usr/lib/libiptc.a
all: bw-stats bwmond bwsim


bw-stats: bw-stats.o 
	$(CC) $(LDFLAGS) -o $@ $^  $(LIBS)
bwmond: bwmond.o 
	$(CC) $(LDFLAGS) -o $@ $^  $(LIBS) $(LIBIPTC)
bwsim: bwsim.o
	$(CC) $(LDFLAGS) -o $@ $^  $(LIBS)
bw-stats.o: bw-stats.c
	$(CC) $(CFLAGS) -c $^
bwmond.o: bwmond.c
	$(CC) $(CFLAGS) -c $^
bwsim.o: bwsim.c
	$(CC) $(CFLAGS) -c $^

clean:
	rm -rf .*sw* *~ *.o bw-stats bwmond bwsim
