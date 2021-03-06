# Quick and dirty and relatively generic Makefile
# Generated by generate_makefile.sh
# By Joel Savitz <jsavitz@redhat.com>

CC 	= gcc
CFLAGS  = -gdwarf-4 -Wall -Werror
OBJECTS = main.o
BIN	= app

all: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(BIN)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONEY: clean
clean:
	rm -rf $(BIN) $(OBJECTS)
