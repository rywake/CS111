#!/bin/bash

for threads in {1,2,4,8,12}
do

    for it in {10,20,40,80,100,1000,10000,100000}
    do
         ./lab2_add --iterations=$it --threads=$threads --yield >> lab2_add.csv
    done

    ./lab2_add --iterations=1000 --threads=$threads --yield --sync=s >> lab2_add.csv
    ./lab2_add --iterations=10000 --threads=$threads --yield --sync=m >> lab2_add.csv
    ./lab2_add --iterations=10000 --threads=$threads --yield --sync=c >> lab2_add.csv

done

for threads in {2,4,8,12}
do

    for it in {100,1000,10000,100000}
    do
         ./lab2_add --iterations=$it --threads=$threads >> lab2_add.csv
    done
done

for sync in {1,2,4,8,12}
do

    for it in {10,20,40,80,100,1000,10000,100000}
    do
    ./lab2_add --iterations=$it --threads=$sync >> lab2_add.csv
    ./lab2_add --iterations=$it --threads=$sync --sync=m >> lab2_add.csv
    ./lab2_add --iterations=$it --threads=$sync --sync=s >> lab2_add.csv
    ./lab2_add --iterations=$it --threads=$sync --sync=c >> lab2_add.csv
    done
done
