all: bw_query
bw_query: ipt_bwctl.o bw_query.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
bw_query.o: bw_query.c
	$(CC) $(CFLAGS)  -c $^ -o $@
ipt_bwctl.o: ipt_bwctl.c
	$(CC) $(CFLAGS)  -c $^ -o $@
clean:
	rm -rf bw_query *.o *~ .*sw*
