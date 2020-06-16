
build:
	gcc -Wall -Wextra -g -pthread -o lab2_add lab2_add.c
	gcc -Wall -Wextra -g -pthread -o lab2_list lab2_list.c SortedList.c

tests: build
	rm -f lab2_list.csv lab2_add.csv
	-./add_data.sh
	-./add_list.sh

graphs: tests
	-./lab2_add.gp
	-./lab2_list.gp

dist: graphs
	tar -czf lab2a-904975422.tar.gz lab2_add.c SortedList.h SortedList.c lab2_list.c lab2_add.csv lab2_list.csv lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png add_data.sh add_list.sh README Makefile ./lab2_list.gp ./lab2_add.gp


clean:
	rm -f lab2a-904975422.tar.gz lab2_add lab2_list

