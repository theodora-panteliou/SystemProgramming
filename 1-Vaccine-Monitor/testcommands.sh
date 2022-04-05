#!/bin/bash

rm -f testcommands.txt; touch testcommands.txt

i=0
while read -r line; do
    IFS=' ' read -ra words <<< "$line"
#/vaccineStatusBloom citizenID virusName
    echo "/vaccineStatusBloom ${words[0]} ${words[5]}" >> testcommands.txt
#/vaccineStatus citizenID virusName 
    echo "/vaccineStatus ${words[0]} ${words[5]}" >> testcommands.txt
#/vaccineStatus citizenID 
    echo "/vaccineStatus ${words[0]}" >> testcommands.txt
#/populationStatus [country] virusName date1 date2
    echo "/populationStatus ${words[4]} ${words[5]} 01-01-2000 02-04-2021" >> testcommands.txt
#/populationStatus [country] virusName date1 date2
    echo "/populationStatus ${words[5]} 01-01-2000 02-04-2021" >> testcommands.txt
#/popStatusByAge [country] virusName date1 date2
    echo "/popStatusByAge ${words[4]} ${words[5]} 01-01-2000 02-04-2021" >> testcommands.txt
#/popStatusByAge [country] virusName date1 date2
    echo "/popStatusByAge ${words[5]} 01-01-2000 02-04-2021" >> testcommands.txt
#/list-nonVaccinated-Persons virusName
    echo "/list-nonVaccinated-Persons ${words[5]}" >> testcommands.txt
#/vaccinateNow citizenID firstName lastName country age virusName
    echo -n "/vaccinateNow " >> testcommands.txt
    for j in 0 1 2 3 4 5; do
        echo -n "${words[j]} " >> testcommands.txt
    done
    echo >> testcommands.txt
    ((i=i+1))
    echo "/vaccineStatus ${words[0]}" >> testcommands.txt
    if [ "$i" -gt 10 ]; then
        echo "/exit" >> testcommands.txt
        break
    fi
done < "inputFile.txt"