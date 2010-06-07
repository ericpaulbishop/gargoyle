# A long time ago, far, far away, under Solaris, you needed to
#    CFLAGS += -xO2 -Xc
#    LDLIBS += -lnsl -lsocket
# To cross-compile
#    CC = arm-linux-gcc
# To check for lint
# -Wundef not recognized by gcc-2.7.2.3
CFLAGS += -W -Wall -Wpointer-arith  -Wshadow -Waggregate-return -Winline -Wwrite-strings -Wstrict-prototypes


# CFLAGS += -DPRECISION_SIOCGSTAMP
CFLAGS += -DENABLE_DEBUG
CFLAGS += -DENABLE_REPLAY
CFLAGS += -DUSE_OBSOLETE_GETTIMEOFDAY




all: ntpclient adjtimex

test: ntpclient
	./ntpclient -d -r <test.dat

ntpclient: ntpclient.o phaselock.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 


adjtimex: adjtimex.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^


clean:
	rm -f ntpclient adjtimex *.o
