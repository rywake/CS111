
all: lab0.c
	gcc -Wall -Wextra -g -o lab0 lab0.c 

clean: 
	rm -f lab0 lab0-904975422.tar.gz

check:
	./testScript.sh

dist: lab0 lab0.c
	tar -czf lab0-904975422.tar.gz README lab0.c Makefile testScript.sh backtrace.png breakpoint.png


