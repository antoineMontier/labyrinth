# make && ./main
# source files list

SRC = $(wildcard *.c)


OBJ = $(addprefix build/,$(SRC:.c=.o))
DEP = $(addprefix build/,$(SRC:.c=.d))

# compilator name
CC = gcc

# executable name
EXE = main

# compilation flages (CFLAGS) and links edition (LDFLAGS)
CFLAGS += -Wall -g
LDFLAGS = #-pg to create a library

# principal rule : make the executable file
all: $(OBJ)
	$(CC) -o $(EXE) $^ $(LDFLAGS)

# standart rule to make a .o file from a .c file
build/%.o: %.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ -c $<

# standart rule to clean
clean:
	rm -rf build core *.gch

# automatic inclution of the dependencies
-include $(DEP)