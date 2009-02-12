#!/bin/sh

CC="g++"

config=$1
file_1=$2
file_2=$3
file_3=$4

if [ ! "$config" = "gargoyle" ] && [ ! "$config" = "openwrt" ] && [ ! "$config" = "fonera" ] ; then
	echo "ERROR: invalid config specified, must be gargoyle, openwrt or fonera"
	echo "Usage: sh build-all-in-one.sh [config] [necessary files]"
	exit;
fi


make bin2c


echo "#include \"fon-flash.h\"" > install_binaries.h

if [ "$config" = "gargoyle" ] || [ "$config" = "openwrt" ] ; then
	echo "flash_configuration* default_conf = get_gargoyle_configuration();" >> install_binaries.h
else
	echo "flash_configuration* default_conf = get_fonera_configuration();" >> install_binaries.h
fi

echo "extern unsigned char  _binary_1_data[];" >> install_binaries.h
echo "extern unsigned long  _binary_1_size;" >> install_binaries.h
echo "extern unsigned char  _binary_2_data[];" >> install_binaries.h
echo "extern unsigned long  _binary_2_size;" >> install_binaries.h
echo "extern unsigned char  _binary_3_data[];" >> install_binaries.h
echo "extern unsigned long  _binary_3_size;" >> install_binaries.h



if [ -z "$file_1" ] ; then
	echo "#include <stdlib.h>" > "file_1.c"
	echo "unsigned char* _binary_1_data = NULL;" >> "file_1.c"
	echo "unsigned long  _binary_1_size = 0;" >> "file_1.c"
else
	if [ ! -e "$file_1" ] ; then
		echo "ERROR: file $file_1 does not exist"
		exit
	fi
	./bin2c "$file_1" "1"
	mv "1.c" file_1.c
fi
if [ -z "$file_2" ] ; then
	echo "#include <stdlib.h>" > "file_2.c"
	echo "unsigned char* _binary_2_data = NULL;" >> "file_2.c"
	echo "unsigned long  _binary_2_size = 0;" >> "file_2.c"
else
	if [ ! -e "$file_2" ] ; then
		echo "ERROR: file $file_2 does not exist"
		exit
	fi
	./bin2c "$file_2" "2"
	mv "2.c" file_2.c
fi
if [ -z "$file_3" ] ; then
	echo "#include <stdlib.h>" > "file_3.c"
	echo "unsigned char* _binary_3_data = NULL;" >> "file_3.c"
	echo "unsigned long  _binary_3_size = 0;" >> "file_3.c"
else
	if [ ! -e "$file_3" ] ; then
		echo "ERROR: file $file_3 does not exist"
		exit
	fi

	./bin2c "$file_3" "3"
	mv "3.c" file_3.c
fi
$CC -c file_1.c -o file_1.o
$CC -c file_2.c -o file_2.o
$CC -c file_3.c -o file_3.o

make "all-in-one"
