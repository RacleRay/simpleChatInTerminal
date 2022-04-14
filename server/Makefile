CC = gcc
CFLAGS = -pedantic -Wall -fsanitize=address -lpthread -lncurses -I./src/ $(DEBUG)

SRC = $(wildcard ./src/*.c)
OBJS = $(patsubst %.c, %.o, $(SRC))

.PHONY: all build clean

all: clean build

build: server

./src/%.o: ./src/%.c
	$(CC)  -c $^ -o $@  $(CFLAGS)

server.o: server.c
	$(CC)  -c $^ -o $@  $(CFLAGS)

server: server.o $(OBJS)
	$(CC)  -o $@ $^  $(CFLAGS)

debug:
	$(MAKE) DEBUG="-g -D_D"

clean:
	rm -rf $(OBJS) server server.o

