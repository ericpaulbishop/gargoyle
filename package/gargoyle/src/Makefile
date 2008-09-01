prefix=/usr
DESTDIR=

LIBS:= -luci -lericstools


all: gargoyle_header_footer 

gargoyle_header_footer: gargoyle_header_footer.o  
	$(CC) $(LDFLAGS) gargoyle_header_footer.o  -o gargoyle_header_footer $(LIBS)
gargoyle_header_footer.o: gargoyle_header_footer.c
	$(CC) $(CFLAGS) -c gargoyle_header_footer.c

clean:
	touch gargoyle_header_footer tmp.o tmp~ .tmp.swp
	rm *.o *~ .*sw* gargoyle_header_footer 
