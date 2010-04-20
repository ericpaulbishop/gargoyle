GHF_LIBS:= -luci -lericstools
GSV_LIBS:= -lcrypt -lericstools

all: gargoyle_header_footer gargoyle_session_validator 

gargoyle_header_footer: gargoyle_header_footer.o  
	$(CC) $(LDFLAGS) gargoyle_header_footer.o  -o gargoyle_header_footer $(GHF_LIBS)
gargoyle_header_footer.o: gargoyle_header_footer.c
	$(CC) $(CFLAGS) -c gargoyle_header_footer.c

gargoyle_session_validator: gargoyle_session_validator.o sha256.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(GSV_LIBS)

gargoyle_session_validator.o: gargoyle_session_validator.c
	$(CC) $(CFLAGS) -c $^ -o $@
sha256.o: sha256.c
	$(CC) $(CFLAGS) -c $^ -o $@
clean:
	rm -rf *.o *~ .*sw* gargoyle_header_footer gargoyle_session_validator sha256 
