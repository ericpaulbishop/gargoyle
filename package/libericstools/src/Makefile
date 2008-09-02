all: erics_tools

test: test.c erics_tools
	$(CC) $(CFLAGS) $(LDFLAGS) test.c -o test -lericstools
erics_tools: priority_queue.o string_map.o string_util.o
	$(CC) -shared $(LDFLAGS) $(CFLAGS) -fPIC -Wl,-soname,libericstools.so.1  priority_queue.o string_map.o string_util.o -o libericstools.so.1
	ln -s libericstools.so.1 libericstools.so
priority_queue.o: priority_queue.c erics_tools.h
	$(CC) $(CFLAGS) -fPIC -c priority_queue.c
string_map.o: string_map.c erics_tools.h
	$(CC) $(CFLAGS) -fPIC -c string_map.c
string_util.o: string_util.c erics_tools.h
	$(CC) $(CFLAGS) -fPIC -c string_util.c
clean:
	touch tmp.o tmp.so tmp~ .tmpswp 
	rm *.o *.so* .*sw* *~ 
