#!/bin/sh
if [ "$1" == "" ];then
	echo "please input program name, no endstring!"
	exit -1
fi

fd=`echo $1 | sed 's/\.s//g'`
rm -f $fd

echo "creat bin file: $fd"
as -gstabs -o $fd.o $fd.s
ld -dynamic-linker /lib/ld-linux.so.2 -o $fd -lc $fd.o
rm $fd.o

exit 0
