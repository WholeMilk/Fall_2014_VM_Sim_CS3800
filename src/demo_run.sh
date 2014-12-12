#!/bin/bash
g++ memorysimulator.c -o memorysimulator &&
echo Compiling memorysimulator success !&&
echo
# ./memorysimulator -help &&
for x in 1 2 4 8 16;
do
	for y in 'fifo' 'lru' 'clock';
	do
		for z in 0 1;
		do
			./memorysimulator programlist.txt programtrace.txt $x $y $z -demo
		done
	done
done
