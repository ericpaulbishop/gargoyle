all: bw_query bw_set
bw_set: ipt_bwctl.o bw_set.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
bw_query: ipt_bwctl.o bw_query.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
bw_set.o: bw_set.c
	$(CC) $(CFLAGS)  -c $^ -o $@
bw_query.o: bw_query.c
	$(CC) $(CFLAGS)  -c $^ -o $@
ipt_bwctl.o: ipt_bwctl.c
	$(CC) $(CFLAGS)  -c $^ -o $@
clean:
	rm -rf bw_query bw_set *.o *~ .*sw*
