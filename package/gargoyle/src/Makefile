prefix=/usr
DESTDIR=

LIBS:= -luci


all: gargoyle_header_footer 

gargoyle_header_footer: gargoyle_header_footer.o string_util.o 
	$(CC) $(LDFLAGS) gargoyle_header_footer.o string_util.o -o gargoyle_header_footer $(LIBS)
gargoyle_header_footer.o: gargoyle_header_footer.c
	$(CC) $(CFLAGS) -c gargoyle_header_footer.c
string_util.o: string_util.c
	$(CC) $(CFLAGS) -c string_util.c

clean:
	touch gargoyle_header_footer tmp.o tmp~ .tmp.swp
	rm *.o *~ .*sw* gargoyle_header_footer 
