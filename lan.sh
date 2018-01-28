#!/bin/bash

j=""
num=0
result=`ifconfig | expand | cut -c1-8 | sort | uniq -u | awk -F: '{print $1;}'`
for i in $result
do
    tmp=`LANG=C ifconfig $i | sed -En 's/127.0.0.1//;s/.*inet (addr:)?(([0-9]*\.){3}[0-9]*).*/\2/p'`
    if [ -z "$tmp" ]; then
        continue
    fi
    j="$j $i"
    (( num++ ))
done

if [ $num -eq 0 ]; then
    echo "error: No network interface available"
    exit -1
fi

if [ $num -ne 1 ]; then
    echo "select network interface" >&2
    select i in $j; do
        j=$i
        break
    done
fi

host=`LANG=C ifconfig $j | sed -En 's/127.0.0.1//;s/.*inet (addr:)?(([0-9]*\.){3}[0-9]*).*/\2/p'`
echo -e "host:\t"$host

