CFLAGS=-Wall -pedantic
DEBUG=-g3
CC=gcc
OBJ=$(patsubst %.c, %.o, $(SRC))

.PHONY: all clean talkie
all: server client

server: src/server.c
	$(CC) $(CFLAGS) -L. $< -o server -lncurses -lncw

client: src/client.c
	$(CC) $(CFLAGS) -L. $< -o client -lncurses -lncw

clean:
	rm -f server client

