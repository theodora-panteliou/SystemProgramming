#!/bin/bash

input_file="outputfile.txt"
declare -i count
count=0
declare -i i
i=0
while read -r line; do
    if [[ "$line" = *"consumer exit recs read:"* ]]; then
        echo $line 
        echo ${line##* }
        count+=${line##* }
        i+=1
    fi
done < "$input_file"
echo $i
echo $count
