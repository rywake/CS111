#!/bin/bash

for it in {10,100,1000,10000,20000}
do
    ./lab2_list --iterations=$it --threads=1 >> lab2_list.csv
done


for it in {1,10,100,1000}
do
    for threads in {2,4,8,12}
    do
	./lab2_list --iterations=$it --threads=$threads  >> lab2_list.csv
	./lab2_list --iterations=$it --threads=$threads >> lab2_list.csv
	./lab2_list --iterations=$it --threads=$threads >> lab2_list.csv
	./lab2_list --iterations=$it --threads=$threads >> lab2_list.csv
	./lab2_list --iterations=$it --threads=$threads >> lab2_list.csv
	./lab2_list --iterations=$it --threads=$threads >> lab2_list.csv
    done
done

for it in {1,2,4,8,16,32}
do
    for threads in {1,2,4,8,12}
    do
    ./lab2_list --iterations=$it --threads=$threads --yield=i >> lab2_list.csv
    ./lab2_list --iterations=$it --threads=$threads --yield=il >> lab2_list.csv
    ./lab2_list --iterations=$it --threads=$threads --yield=dl >> lab2_list.csv
    ./lab2_list --iterations=$it --threads=$threads --yield=l >> lab2_list.csv
    ./lab2_list --iterations=$it --threads=$threads --yield=d >> lab2_list.csv

    done
done


for it in {10,20,100,500,1000}
do
    for threads in {1,2,4,8,12,16,24}
    do
     ./lab2_list --iterations=$it --threads=$threads --sync=s >> lab2_list.csv
     ./lab2_list --iterations=$it --threads=$threads --sync=m >> lab2_list.csv
    done
done


for it in {1,2,4,8,16,32}
do
    for threads in {2,4,8,12}
    do
    ./lab2_list --iterations=$it --threads=$threads --sync=m --yield=i >> lab2_list.csv
    ./lab2_list --iterations=$it --threads=$threads --sync=m --yield=d >> lab2_list.csv
    ./lab2_list --iterations=$it --threads=$threads --sync=m --yield=il >> lab2_list.csv
    ./lab2_list --iterations=$it --threads=$threads --sync=m --yield=dl >> lab2_list.csv
    ./lab2_list --iterations=$it --threads=$threads --sync=m --yield=idl >> lab2_list.csv
    ./lab2_list --iterations=$it --threads=$threads --sync=m --yield=l >> lab2_list.csv 
    ./lab2_list --iterations=$it --threads=$threads --sync=m >> lab2_list.csv

    ./lab2_list --iterations=$it --threads=$threads --sync=s --yield=i >> lab2_list.csv
    ./lab2_list --iterations=$it --threads=$threads --sync=s --yield=d >> lab2_list.csv
    ./lab2_list --iterations=$it --threads=$threads --sync=s --yield=il >> lab2_list.csv
    ./lab2_list --iterations=$it --threads=$threads --sync=s --yield=dl >> lab2_list.csv
    ./lab2_list --iterations=$it --threads=$threads --sync=s --yield=idl >> lab2_list.csv
    ./lab2_list --iterations=$it --threads=$threads --sync=s --yield=l >> lab2_list.csv
    ./lab2_list --iterations=$it --threads=$threads --sync=s >> lab2_list.csv

    done
done


