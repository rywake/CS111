#!/bin/bash

for threads in {1,2,4,8,12,16,24}
do
    ./lab2_list --iterations=1000 --threads=$threads --sync=m >> lab2b_list.csv
    ./lab2_list --iterations=1000 --threads=$threads --sync=s >> lab2b_list.csv
done

for threads in {1,4,8,12,16}
do
    for it in {1,2,4,8,16}
    do
	./lab2_list --iterations=$it --threads=$threads --lists=4 --yield=id >> lab2b_list.csv
    done
done

for threads in {1,4,8,12,16}
do
    for it in {10,20,40,80}
    do
        ./lab2_list --iterations=$it --threads=$threads --lists=4 --yield=id --sync=s >> lab2b_list.csv
	./lab2_list --iterations=$it --threads=$threads --lists=4 --yield=id --sync=m >> lab2b_list.csv
    done
done

for threads in {1,2,4,8,12}
do
    for lists in {4,8,16}
    do
	 ./lab2_list --iterations=1000 --threads=$threads --lists=$lists --sync=s >> lab2b_list.csv
	 ./lab2_list --iterations=1000 --threads=$threads --lists=$lists --sync=m >> lab2b_list.csv
    done
done
