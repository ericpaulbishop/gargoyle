all: erics_tools
VERSION=1


ifeq ($(CC),)
  CC=gcc
endif

ifeq ($(LD),)
  LD=ld
endif

ifeq ($(AR),)
  AR=ar
endif

ifeq ($(RANLIB),)
  RANLIB=ranlib
endif



OS=$(shell uname)
ifeq ($(OS),Darwin)
  LINK=$(LD)
  SHLIB_EXT=dylib
  SHLIB_FLAGS=-dylib
  SHLIB_FILE=libericstools.$(SHLIB_EXT).$(VERSION)
else
  LINK=$(CC)
  SHLIB_EXT=so
  SHLIB_FILE=libericstools.$(SHLIB_EXT).$(VERSION)
  SHLIB_FLAGS=-shared -Wl,-soname,$(SHLIB_FILE)
endif

WARNING_FLAGS=-Wall -Wstrict-prototypes -pedantic
MINIMAL_WARNING_FLAGS=-Wall -Wstrict-prototypes

test_list_and_queue: test_list_and_queue.c libericstools.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
test_list_and_queue.o: test_list_and_queue.c
	$(CC) $(CFLAGS) $(MINIMAL_WARNING_FLAGS) -o $@ -c $^

test_string: test_string.o libericstools.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
test_string.o: test_string.c
	$(CC) $(CFLAGS) $(MINIMAL_WARNING_FLAGS) -c $^ -o $@


test_map: test_map.o libericstools.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
test_map.o: test_map.c
	$(CC) $(CFLAGS) $(MINIMAL_WARNING_FLAGS) -c $^ -o $@




all: erics_tools

erics_tools: libericstools.$(SHLIB_EXT) libericstools.a


libericstools.a: list_static.o priority_queue_static.o tree_map_static.o string_util_static.o
	if [ -e $@ ] ; then rm $@ ; fi
	$(AR) rc $@ $^
	$(RANLIB) $@
	

libericstools.$(SHLIB_EXT) : list_dyn.o priority_queue_dyn.o tree_map_dyn.o string_util_dyn.o
	if [ -e libericstools.$(SHLIB_EXT) ] ; then rm libericstools.$(SHLIB_EXT)* ; fi
	$(LINK) $(LDFLAGS) $(CFLAGS) $(SHLIB_FLAGS) -o $(SHLIB_FILE) $^ -lc
	ln -s $(SHLIB_FILE) libericstools.$(SHLIB_EXT)

list_dyn.o: list.c
	$(CC) $(CFLAGS) -fPIC $(WARNING_FLAGS) -o $@ -c $^
priority_queue_dyn.o: priority_queue.c
	$(CC) $(CFLAGS) -fPIC $(WARNING_FLAGS) -o $@ -c $^
tree_map_dyn.o: tree_map.c 
	$(CC) $(CFLAGS) -fPIC $(WARNING_FLAGS) -o $@ -c $^
string_util_dyn.o: string_util.c 
	$(CC) $(CFLAGS) -fPIC $(WARNING_FLAGS) -o $@ -c $^


list_static.o: list.c
	$(CC) $(CFLAGS) $(WARNING_FLAGS) -o $@ -c $^
priority_queue_static.o: priority_queue.c
	$(CC) $(CFLAGS) $(WARNING_FLAGS) -o $@ -c $^
tree_map_static.o: tree_map.c 
	$(CC) $(CFLAGS) $(WARNING_FLAGS) -pedantic -o $@ -c $^
string_util_static.o: string_util.c 
	$(CC) $(CFLAGS) $(WARNING_FLAGS) -o $@ -c $^



clean:
	touch tmp.a tmp.o tmp.$(SHLIB_EXT) tmp~ .tmp.swp test_map test_string test_list_and_queue
	rm *.a *.o *.$(SHLIB_EXT)* .*sw* *~ test_map test_string test_list_and_queue
