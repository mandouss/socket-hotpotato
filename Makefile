#
#
CC=gcc
CFLAGS=-g -std=gnu99

# comment line below for Linux machines
#LIB= -lsocket -lnsl

all: ringmaster player

ringmaster:	ringmaster.o
	$(CC) $(CFLAGS) -o $@ ringmaster.o $(LIB)

player:	player.o
	$(CC) $(CFLAGS) -o $@ player.o $(LIB)

ringmaster.o:	ringmaster.c

player.o:	player.c 

clean:
	rm -f ringmaster ringmaster.o ringmaster~ player player.o player~