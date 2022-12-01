CC=gcc
CFLAGS=-Wall -Wextra -std=gnu11 -g -O3 -fsanitize=leak -fsanitize=address

# := evaulate every time 
VPATH:=helpers/ lib/ interface/ 
.PHONY:all


all: shell # first objective to call 

# liage 
shell: lib.o helpers.o interface.o shell.o
	$(CC) $(CFLAGS) $^ -o $@ 

# $@ : repalce objective name 
lib.o: lib/lib.c lib/lib.h
	$(CC) $(CFLAGS) -c lib/lib.c -o $@

helpers.o: helpers/helpers.c helpers/helpers.h
	$(CC) $(CFLAGS) -c helpers/helpers.c -o $@

interface.o: interface/interface.c interface/interface.h 
	$(CC) $(CFLAGS) -c interface/interface.c -o $@


run:
	./shell
cls:
	clear
clean:
	rm -rf *.o shell
	$(MAKE) cls

rebuild: clean all
	$(MAKE) clean # call clean objective 
	$(MAKE) shell