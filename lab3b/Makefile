default:
	ln -s lab3b.py lab3b

.PHONY: dist
dist: lab3b.py Makefile README
	tar -czf lab3b-805133939.tar.gz $^

.PHONY: clean
clean:
	rm -rf lab3b *.tar.gz