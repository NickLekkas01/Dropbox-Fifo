#!/bin/bash

if [ "$4" = "" ]; then
	echo "We dont have 5 parameters"
	exit -1
fi
if [ ! -d "$1" ]; then
	echo "File doesnt exist"
	exit -1
fi
cd $1
for i in $(seq 1 $3); do
	NUMBER=$(cat /dev/urandom | tr -dc '1-8' | fold -w 256 | head -n 1 | sed -e 's/^0*//' | head --bytes 1)
	MYDIR[$i]=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $NUMBER | head -n 1)
done

index=0
for i in $(seq 1 $3); do
	mkdir ${MYDIR[$i]}
	cd ${MYDIR[$i]}
	index=$((index + 1))
	if [ $(($i % $4)) == 0 ]
	then
		for j in $(seq 1 $index); do
			cd ..
		done
		index=0
	fi
done

for i in $(seq 1 $index); do
	cd ..
done
	
index=0
index2=1
for i in $(seq 1 $2); do
	if [ $index == $4 ] || [ $(($(($index2 - 1)) % $3)) == 0 ]
	then
		for j in $(seq 1 $index); do
			cd ..
		done
		index=0
	fi
	
	if [ $(($i % $(($3 + 1)))) != 1 ]
	then
		cd ${MYDIR[$(($(($index2 - 1)) % $3 + 1))]}
		index=$(($index + 1))
		index2=$(($index2 + 1))
	fi
	touch f$i
	NUMBER=$(cat /dev/urandom | tr -dc '0-9' | fold -w 256 | head -n 1 | sed -e 's/^0*//' | head --bytes 6)
	while [ $NUMBER -lt 1000 ] || [ $NUMBER -gt 128000 ]; do
		NUMBER=$(cat /dev/urandom | tr -dc '0-9' | fold -w 256 | head -n 1 | sed -e 's/^0*//' | head --bytes 6)
	done
	#echo $NUMBER
	for j in $(seq 1 $(($NUMBER / 1000))); do
		Data=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 1000 | head -n 1)
		echo -n $Data >>f$i
	done
	if [ $(($NUMBER % 1000)) != 0 ]
	then
		Data=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $(($NUMBER % 1000)) | head -n 1)
		echo -n $Data >>f$i
	fi
	
done
