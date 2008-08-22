LIBS:=-lpcap

CFLAGS:= -Wall -Os
#CFLAGS:= $(CFLAGS) -DISBSD

all: webmon
webmon: webmon.o string_map.o string_util.o
	$(CC) $(CFLAGS) $(LDFLAGS) webmon.o string_map.o string_util.o -o $@ $(LIBS)
webmon.o: webmon.c
	$(CC) $(CFLAGS) -c webmon.c
string_map.o: string_map.c
	$(CC) $(CFLAGS) -c string_map.c
string_util.o: string_util.c
	$(CC) $(CFLAGS) -c string_util.c
clean:
	touch tmp.o tmp~ .tmp.swp webmon 
	rm *.o *~ .*sw* webmon
