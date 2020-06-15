default:
	gcc -Wall -Wextra -lssl -lcrypto -lmraa -lm -g -o lab4c_tcp lab4c_tcp.c
	gcc -Wall -Wextra -lssl -lcrypto -lmraa -lm -g -o lab4c_tls lab4c_tls.c

clean:
	rm -f lab4c_tls lab4c_tcp lab4c-904975422.tar.gz

dist:
	tar -czf lab4c-904975422.tar.gz README Makefile lab4c_tcp.c lab4c_tls.c

