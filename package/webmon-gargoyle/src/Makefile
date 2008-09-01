LIBS:=-lpcap -lericstools

CFLAGS:= -Wall -Os
#CFLAGS:= $(CFLAGS) -DISBSD

all: webmon
webmon: webmon.o 
	$(CC) $(CFLAGS) $(LDFLAGS) webmon.o -o $@ $(LIBS)
webmon.o: webmon.c
	$(CC) $(CFLAGS) -c webmon.c
clean:
	touch tmp.o tmp~ .tmp.swp webmon 
	rm *.o *~ .*sw* webmon
