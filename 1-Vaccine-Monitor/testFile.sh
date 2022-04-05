#!/bin/bash

# ./testFile.sh virusesFile countriesFile numLines duplicatesAllowed 
if [ "$#" -ne 4 ]; then
    echo "Expected 4 arguments"
    exit 0
fi
outputfile=inputFile.txt
rm -f $outputfile; touch $outputfile
virusesFile=$1
countriesFile=$2
numLines=$3
duplicatesAllowed=$4
if [ "${duplicatesAllowed}" -eq 0 ] && [ "${numLines}" -gt 10000 ]; then
    echo "Cannot create more than 10000 unique records for 4-digit IDs. Allow duplicates or choose 1-10000 lines."
    exit 1
fi
declare -a countries
while read -r line; do 
    if [[ "${line}" =~ [^a-zA-Z] ]]; then
        continue
    fi
    countries+=("${line}")
done < "$countriesFile"
countriesnum=${#countries[@]}

declare -a viruses
while read -r line; do
    if [[ "${line}" =~ [^a-zA-Z1-9-] ]]; then  
        continue
    fi
    if [[ "${line}" =~ \ |\' ]]; then
        continue
    fi
    viruses+=("${line}")
done < "$virusesFile"
virusesnum=${#viruses[@]}

declare -a randomids
randomids=($(shuf -i 0-9999))
LIMIT=$numLines

declare -a lowerletters
declare -a upperletters
upperletters=(A B C D E F G H I J K L M N O P Q R S T U V W X Y Z)
lowerletters=("a" "b" "c" "d" "e" "f" "g" "h" "i" "j" "k" "l" "m" "n" "o" "p" "q" "r" "s" "t" "u" "v" "w" "x" "y" "z")

declare -a record_array
declare -i counter_unique
counter_unique=0

for ((i=1; i<=LIMIT; i++))
do  
# echo $i 
    if [[ ("${duplicatesAllowed}" != 0  &&  $(($RANDOM % 100)) -lt 10  &&  "$i" != 1) || ("$counter_unique" -gt 9999) ]] ; then #possibility 1/10 that a record is duplicate
        echo -n "${record_array[$RANDOM%$counter_unique]}"  >> $outputfile
    else 
    # id
        id=${randomids[${counter_unique}%10000]}
        record="${id} "
    # first name
        charnum=$((2 + $RANDOM % 10)) # 11 characters
        record+="${upperletters[$RANDOM%26]}"
        for ((j=1; j<=$charnum; j++))
        do
            record+="${lowerletters[$RANDOM%26]}"
        done
        record+=" "
    # last name
        charnum=$((2 + $RANDOM % 10)) # 11 characters
        record+="${upperletters[$RANDOM%26]}"
        for ((j=1; j<=$charnum; j++))
        do
            record+="${lowerletters[$RANDOM%26]}"
        done
        record+=" "
    # country name
        record+="${countries[$RANDOM%countriesnum]} "
    # age
        age=$((1+ $RANDOM % 119))
        record+="${age} "
    #add record to dictioanry
        record_array+=("$record")
        echo -n "$record" >> $outputfile

        counter_unique=$((counter_unique+1))
    fi
# virus name
    echo -n "${viruses[$RANDOM%virusesnum]} " >> $outputfile

# answer
    if [ "$((RANDOM % 2))" -eq 1 ]; then #50/50 that is a YES or a NO
        echo -n "YES " >> $outputfile
        # date

        # if [ "$((RANDOM % 100))" -lt 10 || ]; then
            year=$((1990 + RANDOM % $((2020-1990))))
            month=$((1 + RANDOM % $((12-1))))
            day=$((1 + RANDOM % $((30-1))))
            if [ $(($month/10)) -eq 0 ]; then
                month="0$month"
            fi
            if [ $(($day/10)) -eq 0 ]; then
                day="0$day"
            fi
            echo -n "${day}-${month}-${year}" >> $outputfile
        # fi
    else 
        echo -n "NO" >> $outputfile

        # if [ "$((RANDOM % 7))" -eq 1 ]; then
        #     year=$((1990 + RANDOM % $((2020-1990))))
        #     month=$((1 + RANDOM % $((12-1))))
        #     day=$((1 + RANDOM % $((30-1))))
        #     if [ $(($month/10)) -eq 0 ]; then
        #         month="0$month"
        #     fi
        #     if [ $(($day/10)) -eq 0 ]; then
        #         day="0$day"
        #     fi
        #     echo -n "${day}-${month}-${year}" >> testfile.txt
        # fi
    fi
    echo >> $outputfile
done