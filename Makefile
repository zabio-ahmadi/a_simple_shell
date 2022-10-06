CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -g -O3 -fsanitize=leak -fsanitize=address  -fsanitize=address

VPATH:=helpers/ lib/ interface/ 

.PHONY:all


all: shell # first objective to call 

shell: lib.o helpers.o interface.o shell.o
	$(CC) $(CFLAGS) $^ -o $@ 

lib.o: lib.c lib.h
helpers.o: helpers.c helpers.h
interface.o: interface.c interface.h 


run:
	./shell
cls:
	clear
clean:
	rm -rf *.o shell
	$(MAKE) cls

rebuild: 
	$(MAKE) clean 
	$(MAKE) shell