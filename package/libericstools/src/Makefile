all: erics_tools

test_string: test_string.o erics_tools
	$(CC) $(CFLAGS) $(LDFLAGS) test_string.o string_util.o -o test_string
test_string.o: test_string.c
	$(CC) $(CFLAGS) -Wall -Wstrict-prototypes -c test_string.c -o test_string.o

test_map: test_map.o erics_tools
	$(CC) $(CFLAGS) $(LDFLAGS) test_map.o tree_map.o -o test_map
test_map.o: test_map.c
	$(CC) $(CFLAGS) -Wall -Wstrict-prototypes -c test_map.c -o test_map.o

erics_tools: priority_queue.o tree_map.o string_util.o
	$(CC) -shared $(LDFLAGS) $(CFLAGS) -fPIC -Wl,-soname,libericstools.so.1  priority_queue.o tree_map.o string_util.o -o libericstools.so.1
	if [ -e libericstools.so ] ; then rm libericstools.so ; fi
	ln -s libericstools.so.1 libericstools.so
priority_queue.o: priority_queue.c erics_tools.h
	$(CC) $(CFLAGS) -fPIC -Wall -Wstrict-prototypes -c priority_queue.c
tree_map.o: tree_map.c erics_tools.h
	$(CC) $(CFLAGS) -fPIC -Wall -Wstrict-prototypes -pedantic -c tree_map.c
string_util.o: string_util.c erics_tools.h
	$(CC) $(CFLAGS) -fPIC -Wall -Wstrict-prototypes -c string_util.c
clean:
	touch tmp.o tmp.so tmp~ .tmp.swp test_map 
	rm *.o *.so* .*sw* *~ test_map 
