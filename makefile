all: sfind.h main.c
	gcc -o sfind sfind.h main.c

test: all
	./sfind ./

valgrind: all
	valgrind ./sfind
