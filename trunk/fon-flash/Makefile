# Copyright (c) 2009, Eric Bishop
# Copyright (c) 2007, Sven-Ola
# All rights reserved.
#
#  This file is free software: you may copy, redistribute and/or modify it
#  under the terms of the GNU General Public License as published by the
#  Free Software Foundation, either version 2 of the License, or (at your
#  option) any later version.
#
#  This file is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

all: fon-flash fon-flash-gui


CC      = g++
AR      = ar
CFLAGS  = -Wall  -I. -Os
OBJS    = uip.o uip_arp.o timer.o clock-arch.o psock.o
OS=$(shell uname)

USEG4=
HAVEG4=

ifeq ($(OS),Darwin)
    USEG4=yes
    HAVEG4=$(shell g++-4.0 --version 2>/dev/null)
endif
ifeq ($(HAVEG4),)
    USEG4=""
endif
ifeq ($(USEG4),yes)
    CC=g++-4.0
    LD=g++-4.0
endif


#CFLAGS += -D_DEBUG


%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

fon-flash-gui: $(OBJS) wx-config fon-flash-gui.o
	$(CC)  -o fon-flash-gui $(OBJS)   fon-flash-gui.o -lpcap `./wx-config --cxxflags --libs --static=yes` 
	if [ "$(OS)" = "Darwin" ] && [ -e fon-flash-gui ] ; then cp -r osx_bundle_files FonFlash.app ; mv fon-flash-gui FonFlash.app/Contents/MacOS/ ; fi

fon-flash-gui.o: wx-config 
	$(CC) -c fon-flash-gui.cpp  -o fon-flash-gui.o `./wx-config --cxxflags  --static=yes` 

wx-config:
	sh build-static-wx.sh



all-in-one: $(OBJS) all-in-one.o file_1.o file_2.o file_3.o
	$(CC) $(CFLAGS) $(OBJS) file_1.o file_2.o file_3.o all-in-one.o -lpcap -o $@
	
all-in-one.o: 
	$(CC) $(CFLAGS) -DINCLUDE_BINARIES -c fon-flash.cpp -o all-in-one.o

bin2c:
	$(CC) bin2c.c -o bin2c

fon-flash: $(OBJS) fon-flash.o Makefile
	$(CC) $(CFLAGS) $(OBJS) fon-flash.o -lpcap -o $@

clean:
	rm -rf *.o *~ *.plg *.ncb fon-flash bin2c file_1.c file_2.c file_3.c install_binaries.h all-in-one fon-flash-gui FonFlash.app

deepclean:
	rm -rf *.o *~ *.plg *.ncb fon-flash bin2c file_1.c file_2.c file_3.c install_binaries.h all-in-one fon-flash-gui *.a *pcap*.h static-pcap pcap wx-config static-wx FonFlash.app
