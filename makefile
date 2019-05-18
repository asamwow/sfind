all: tree.h main.c
	gcc -o tree tree.h main.c

test: all
	./tree -la

valgrind: all
	valgrind ./tree
