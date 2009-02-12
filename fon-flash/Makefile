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

all: fon-flash 


CC      = g++
AR      = ar
CFLAGS  = -Wall  -I. -Os
OBJS    = uip.o uip_arp.o timer.o clock-arch.o psock.o

#CFLAGS += -D_DEBUG

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

all-in-one: $(OBJS) all-in-one.o file_1.o file_2.o file_3.o
	$(CC) $(CFLAGS) $(OBJS) file_1.o file_2.o file_3.o all-in-one.o -lpcap -o $@
	
all-in-one.o: 
	$(CC) $(CFLAGS) -DINCLUDE_BINARIES -c fon-flash.c -o all-in-one.o

bin2c:
	$(CC) bin2c.c -o bin2c

fon-flash: $(OBJS) fon-flash.o Makefile
	$(CC) $(CFLAGS) $(OBJS) fon-flash.o -lpcap -o $@
	strip $@

clean:
	rm -fr *.o *~ *.plg *.ncb fon-flash bin2c all-in-one
