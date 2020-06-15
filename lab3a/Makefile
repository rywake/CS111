# defaults
CC=gcc
WARN=-Wall -Wextra

# catchall object routine
%.o: %.c
	$(CC) $(CFLAGS) -g -c -o $@ $^

# debug object routine
%.d.o: %.c
	$(CC) $(CFLAGS) -g -c -o $@ $^ -D DEBUG

# prod by default
default: prod

# builds with debug symbols and no optimization
debug: libext2.d.o lab3a.d.o
	$(CC) $(CFLAGS) $(WARN) -O0 -g -o lab3a $^

# builds for submission
# debug output is stripped
prod: libext2.o lab3a.o
	$(CC) $(CFLAGS) $(WARN) -O3 -o lab3a $^

# build the tarball
.PHONY: dist
dist: ext2_fs.h libext2.h libext2.c lab3a.c Makefile README
	tar -czf lab3a-805133939.tar.gz $^

# restore directory to a freshly untarred one
.PHONY: clean
clean:
	rm -rf *.tar.gz *.o *.d.o *.h.gch lab3a