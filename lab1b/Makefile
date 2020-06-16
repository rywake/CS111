
default:
	gcc -Wextra -Wall -g -lz -o lab1b-server lab1b-server.c
	gcc -Wextra -Wall -g -lz -o lab1b-client lab1b-client.c

clean:	
	rm -f lab1b-server
	rm -f lab1b-client
	rm -f lab1b-904975422.tar.gz

dist:
	tar -czf lab1b-904975422.tar.gz lab1b-server.c lab1b-client.c Makefile README
