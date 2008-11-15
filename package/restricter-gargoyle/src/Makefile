LIBS:=/usr/lib/libiptc.a -lm 

all: restricter

restricter: restricter.o 
	$(CC) $(LDFLAGS) $(CFLAGS) -o restricter $^ -lericstools $(LIBS)


time-test: time-test.o string_util.o tree_map.o priority_queue.o list.o
	$(CC) $(LDFLAGS) $(CFLAGS)  -o time-test $^
time-test.o: time-test.c
	$(CC) $(CFLAGS)  -Wall -c time-test.c -o time-test.o


restricter.o: restricter.c
	$(CC) $(CFLAGS)  -Wall -c restricter.c -o restricter.o


priority_queue.o: priority_queue.c erics_tools.h
	$(CC) $(CFLAGS) -Wall -c priority_queue.c
list.o: list.c erics_tools.h
	$(CC) $(CFLAGS) -Wall -c list.c
tree_map.o: tree_map.c erics_tools.h
	$(CC) $(CFLAGS) -Wall -c tree_map.c
string_util.o: string_util.c erics_tools.h
	$(CC) $(CFLAGS) -Wall -c string_util.c

clean:
	touch restricter tmp.o tmp~ .tmp.swp
	rm restricter *.o *~ .*sw*
