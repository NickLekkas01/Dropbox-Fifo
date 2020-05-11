#!/bin/bash
array=()
number_of_clients=0
max=$line
min=$line
bytesread=0
byteswritten=0
filesread=0
fileswritten=0
clients_out=0
array+=($line)

while read line
do
    if [ "$line" == "03" ] 
    then
        clients_out=$(($clients_out + 1))
    elif [ "$line" == "00" ] 
    then
        read line
        if [ $line '>' $max ]
        then 
            max=$line
        fi
        if [ $line '<' $min ]
        then 
            min=$line
        fi
        array+=($line)
        number_of_clients=$(($number_of_clients + 1))
    else
        if [ "$line" == "01" ] 
        then
            read line
            read line
            bytesread=$(($bytesread + $line))
            filesread=$(($filesread + 1))
        fi
        if [ "$line" == "02" ] 
        then
            read line
            read line
            byteswritten=$(($byteswritten + $line))
            fileswritten=$(($fileswritten + 1))
        fi
    fi
done < "${1:-/dev/stdin}"
echo ""
echo "Number of Clients ${#array[@]}"
echo "Clients: ${array[@]} "
echo "Max client id: $max"
echo "Min client id: $min"
echo "Bytes written: $byteswritten"
echo "Bytes read: $bytesread"
echo "Files written: $fileswritten"
echo "Files read: $filesread"
echo "Clients out: $clients_out"


