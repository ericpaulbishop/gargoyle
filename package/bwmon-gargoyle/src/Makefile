LIBS:=/usr/lib/libiptc.a -lericstools
CFLAGS:= -Wall

all: bw-stats bwmond bwsim


bw-stats: bw-stats.o 
	$(CC) $(LDFLAGS) -o $@ $^  $(LIBS)
bwmond: bwmond.o 
	$(CC) $(LDFLAGS) -o $@ $^  $(LIBS)
bwsim: bwsim.o
	$(CC) $(LDFLAGS) -o $@ $^ -lericstools
bw-stats.o: bw-stats.c
	$(CC) $(CFLAGS) -c $^
bwmond.o: bwmond.c
	$(CC) $(CFLAGS) -c $^
bwsim.o: bwsim.c
	$(CC) $(CFLAGS) -c $^

clean:
	rm -rf .*sw* *~ *.o bw-stats bwmond bwsim
