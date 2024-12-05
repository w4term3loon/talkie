CFLAGS=-Wall -pedantic
DEBUG=-g3
CC=gcc
OBJ=$(patsubst %.c, %.o, $(SRC))

.PHONY: all clean talkie
all: server client

server: src/server.c
	$(CC) $(CFLAGS) $< -o server

client: src/client.c ncwrap/libncw.a
	$(CC) $(CFLAGS) $< ncwrap/libncw.a -o client -L. -lncurses

ncwrap/libncw.a:
	git submodule init && git submodule update && make --directory=ncwrap

clean:
	rm -f server client

