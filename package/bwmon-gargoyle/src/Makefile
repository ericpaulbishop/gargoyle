LIBS:=/usr/lib/libiptc.a -lericstools
CFLAGS:= -Wall

all: bw-stats bwmond

bw-stats: bw-stats.o 
	$(CC) $(LDFLAGS) -o bw-stats bw-stats.o $(LIBS)
bwmond: bwmond.o 
	$(CC) $(LDFLAGS) -o bwmond bwmond.o $(LIBS)
bw-stats.o: bw-stats.c
	$(CC) $(CFLAGS) -c bw-stats.c
bwmond.o: bwmond.c
	$(CC) $(CFLAGS) -c bwmond.c
clean:
	touch tmp.o bwmond bw-stats
	rm *.o bw-stats bwmond
