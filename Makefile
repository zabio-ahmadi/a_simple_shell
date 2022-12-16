# Compiler to use
CC=gcc

# Compiler flags
CFLAGS=-Wall -Wextra -std=gnu11 -g -O3 -fsanitize=leak -fsanitize=address

# Directories to search for source files
VPATH:=helpers/ lib/ interface/ 

# Declare all as a phony target to always run the recipes for it
.PHONY:all

# Default target to build
all: shell 

# Link object files to create executable
shell: lib.o helpers.o interface.o shell.o
	$(CC) $(CFLAGS) $^ -o $@ 

# Compile lib.c into an object file
lib.o: lib/lib.c lib/lib.h
	$(CC) $(CFLAGS) -c lib/lib.c -o $@

# Compile helpers.c into an object file
helpers.o: helpers/helpers.c helpers/helpers.h
	$(CC) $(CFLAGS) -c helpers/helpers.c -o $@

# Compile interface.c into an object file
interface.o: interface/interface.c interface/interface.h 
	$(CC) $(CFLAGS) -c interface/interface.c -o $@

# Run the shell executable
run:
	./shell

# Clear the terminal screen
cls:
	clear

# Remove object files and executables
clean:
	rm -rf *.o shell
	$(MAKE) cls

# Rebuild the project from scratch
rebuild: clean all
	$(MAKE) clean # call clean objective 
	$(MAKE) shell
