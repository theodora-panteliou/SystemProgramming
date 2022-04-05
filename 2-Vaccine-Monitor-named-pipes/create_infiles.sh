#!/bin/bash

# ./create_infiles.sh inputFile input_dir numFilesPerDirectory

if [ "$#" -ne 3 ]; then
    echo "Expected 3 arguments. './create_infiles.sh inputFile input_dir numFilesPerDirectory'"
    exit 0
fi

input_file=$1
input_dir=$2
num_files_per_directory=$3

if [ ! -e "$input_file" ]; then 
    echo "Input file does not exist."
    exit 0
fi

if [ -d "$input_dir" ]; then # don't overwrite input dir
    echo "Input directory already exists."
    exit 0
fi

mkdir "$input_dir"

# dictionary with key the country and value the file number that the next record 
# for that country has to go to. This achieves Round-Robin
declare -A country_index

# read input file and split the records depending on country
while read -r line; do
    IFS=' ' read -ra words <<< "$line"
    country=${words[3]}
    if [ ! -d "./$input_dir/$country" ]; then 
        mkdir "./$input_dir/$country"
        # for ((i=1; i<=$num_files_per_directory; i++)); do
        #     touch "./$input_dir/${words[3]}/${words[3]}-$i.txt"
        # done
        country_index[$country]=1
    fi

    echo "${line}" >> "./$input_dir/$country/$country-${country_index[$country]}.txt"
    country_index[$country]=$(((${country_index[$country]}%$num_files_per_directory+1)))
done < "$input_file"
