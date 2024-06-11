CFLAGS=-Wall -pedantic
DEBUG=-g3
CC=gcc
OBJ=$(patsubst %.c, %.o, $(SRC))

.PHONY: all clean talkie
all: server client

server: src/server.c
	$(CC) $(CFLAGS) $< -o server

client: src/client.c
	$(CC) $(CFLAGS) $< libncw.a -o client -L. -lncurses

clean:
	rm -f server client

