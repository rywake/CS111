#NAME: Ryan Wakefield
#EMAIL: ryanwakefield@g.ucla.edu
#ID: 904975422

build:
	gcc -Wall -Wextra -g -pthread -L/run_gperftools/lib/lprofiler -o lab2_list lab2_list.c SortedList.c

tests: build
	rm -f lab2b_list.csv
	-./test_script.sh

profile: build
	rm -f ./raw.gperf profile.out
	LD_PRELOAD=/u/ma/ugrad/ryan/cs111/lab2b/run_gperftools/lib/libprofiler.so.0 CPUPROFILE=./raw.gperf ./lab2_list --iterations=1000 --threads=12 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.out
	pprof --list=run_thread ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf

graphs: tests
	-./lab2b.gp

dist: graphs
	tar -czf lab2b-904975422.tar.gz SortedList.h SortedList.c lab2_list.c lab2b.gp lab2_list.c profile.out lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png test_script.sh Makefile README lab2b_list.csv

clean:
	rm -f lab2b-904975422.tar.gz lab2_list

