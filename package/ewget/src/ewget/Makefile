
OS=$(shell uname)
ifeq ($(OS),Darwin)
  CFLAGS:=$(CFLAGS) -arch i386
endif

all: ewget

ewget: ewget.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^  -o $@ -lewget

ewget.o: ewget.c
	$(CC) $(CFLAGS) -c $^ -o $@ 

clean:
	rm -rf ewget *.o *~ .*sw* 
