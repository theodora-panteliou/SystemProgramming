# System Programming
Assignments for the course System Programming at DIT, NKUA that aim to familiarize with system programming methods. 

## Project 1
Vaccine Monitor in C that initializes some structures based on a file of records and then waits for commands. 

Implemented:
* skip list
* bloom filter
* hash table

The file is created randomly with a bash script:
```
./testFile.sh viruses_file countries_file number_of_records duplicates_allowed
```
For example:
```
$./testFile.sh ./testfiles/virusesfile.txt ./testfiles/countries.txt 50000 1
```
Output is created at: `./inputFile.txt`
To run the vaccine monitor app:

```
$make
$./build/vaccineMonitor -c inputFile  -b bloom_size
```

where bloom size is the size of the bloom filter array in bytes. For example:

```
$./build/vaccineMonitor -c inputFile.txt  -b 100000
```

The program reads the file and initializes the structures.
Then you can gine the following commands (arguments inside [] are optional):
* `/vaccineStatusBloom citizenID virusName`
* `/vaccineStatus citizenID virusName`
* `/vaccineStatus citizenID`
* `/populationStatus [country] virusName [date1 date2]`
* `/popStatusByAge [country] virusName [date1 date2]`
* `/insertCitizenRecord citizenID firstName lastName country age virusName YES/NO [date]`
* `/vaccinateNow citizenID firstName lastName country age virusName`
* `/list-nonVaccinated-Persons virusName`
* `/exit`

## Project 2

Purpose of the assignment was to familiarize with: 
* Proceses
* Named Pipes
* System Calls

At first, run the `create_infiles.sh` bash script to create the input direcotory:
```
$./create_infiles.sh inputFile input_directory num_of_records
```
where inputFile is the input and input_directory is the output directory. For example:
```
./create_infiles.sh inputFile.txt input_dir 5
```

To run the vaccine monitor app:

```
$make
$./build/travelMonitor -–m numMonitors -b bufferSize -s sizeOfBloom -i input_dir
```
where numMonitors is the number of the monitor processes, buferSize is the size of buffer for reading through pipes, input_dir is the input directory that was created with the bash script. 

For example:
```
$./build/travelMonitor -i input_dir -s 100000 -b 100 -m 3
```
The program reads from that directory, communicates with processes through named pipes and initializes structures.
Then it waits for the following commands:
* `/travelRequest citizenID date countryFrom countryTo virusName`
* `/travelStats virusName date1 date2 [country]`
* `/addVaccinationRecords country`
* `/searchVaccinationStatus citizenID`
* `/exit`

## Project 3

Purpose of the assignment was to familiarize with: 
* Threads
* Sockets

