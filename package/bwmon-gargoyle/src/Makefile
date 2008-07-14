LIBS:=/usr/lib/libiptc.a
CFLAGS:= -Wall

all: bw-stats bwmond

bw-stats: bw-stats.o string_util.o
	$(CC) $(LDFLAGS) -o bw-stats bw-stats.o string_util.o $(LIBS)
bwmond: bwmond.o priority_queue.o string_util.o
	$(CC) $(LDFLAGS) -o bwmond bwmond.o priority_queue.o string_util.o $(LIBS)
bw-stats.o: bw-stats.c
	$(CC) $(CFLAGS) -c bw-stats.c
bwmond.o: bwmond.c
	$(CC) $(CFLAGS) -c bwmond.c
priority_queue.o: priority_queue.c
	$(CC) $(CFLAGS) -c priority_queue.c
string_util.o: string_util.c
	$(CC) $(CFLAGS) -c string_util.c
clean:
	touch tmp.o bwmond bw-stats
	rm *.o bw-stats bwmond
