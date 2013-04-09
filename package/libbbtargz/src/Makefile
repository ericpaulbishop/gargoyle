all: libbbtargz
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


CFLAGS:=$(CFLAGS) -Os
WARNING_FLAGS=-Wall -Wstrict-prototypes

OS=$(shell uname)
ifeq ($(OS),Darwin)
  LINK=$(LD)
  SHLIB_EXT=dylib
  SHLIB_FLAGS=-dylib
  SHLIB_FILE=libbbtargz.$(SHLIB_EXT).$(VERSION)
  CFLAGS:=$(CFLAGS) -arch i386
else
  LINK=$(CC)
  SHLIB_EXT=so
  SHLIB_FILE=libbbtargz.$(SHLIB_EXT).$(VERSION)
  SHLIB_FLAGS=-shared -Wl,-soname,$(SHLIB_FILE)
endif


all: libbbtargz

libbbtargz: libbbtargz.$(SHLIB_EXT) libbbtargz.a


libbbtargz.a: all_read_static.o concat_path_file_static.o copy_file_static.o copy_file_chunk_static.o gz_open_static.o last_char_is_static.o make_directory_static.o mode_string_static.o parse_mode_static.o safe_strncpy_static.o time_string_static.o unarchive_static.o unzip_static.o wfopen_static.o xfuncs_static.o xreadlink_static.o targz_message_static.o md5_static.o
	if [ -e $@ ] ; then rm $@ ; fi
	$(AR) rc $@ $^
	$(RANLIB) $@
	

libbbtargz.$(SHLIB_EXT) : all_read_dyn.o concat_path_file_dyn.o copy_file_dyn.o copy_file_chunk_dyn.o gz_open_dyn.o last_char_is_dyn.o make_directory_dyn.o mode_string_dyn.o parse_mode_dyn.o safe_strncpy_dyn.o time_string_dyn.o unarchive_dyn.o unzip_dyn.o wfopen_dyn.o xfuncs_dyn.o xreadlink_dyn.o targz_message_dyn.o md5_dyn.o
	if [ -e libbbtargz.$(SHLIB_EXT) ] ; then rm libbbtargz.$(SHLIB_EXT)* ; fi
	$(LINK) $(LDFLAGS) $(SHLIB_FLAGS) -o $(SHLIB_FILE) $^ -lc
	ln -s $(SHLIB_FILE) libbbtargz.$(SHLIB_EXT)


%_dyn.o: %.c
	$(CC) $(CFLAGS) -fPIC $(WARNING_FLAGS) -o $@ -c $^

%_static.o: %.c
	$(CC) $(CFLAGS) $(WARNING_FLAGS) -o $@ -c $^



clean:
	rm -rf *.a *.o .*sw* *~ 
	if [ "$(SHLIB_EXT)" != "" ] ; then rm -rf *.$(SHLIB_EXT)* ; fi

