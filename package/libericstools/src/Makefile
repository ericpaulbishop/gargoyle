CFLAGS:= -Wall

all: erics_tools

erics_tools: priority_queue.o string_map.o string_util.o
	$(CC) -shared $(LDFLAGS) $(CFLAGS)  priority_queue.o string_map.o string_util.o -o libericstools.so 
priority_queue.o: priority_queue.c erics_tools.h
	$(CC) $(CFLAGS) -c priority_queue.c
string_map.o: string_map.c erics_tools.h
	$(CC) $(CFLAGS) -c string_map.c
string_util.o: string_util.c erics_tools.h
	$(CC) $(CFLAGS) -c string_util.c
clean:
	touch tmp.o tmp.so tmp~ .tmpswp 
	rm *.o *.so .*sw* *~
