CC = gcc
UI = -DUI
CFLAGS = -pedantic -Wall -fsanitize=address -lpthread -lncursesw -I./src/ -D_CLIENT $(UI) $(DEBUG)

SRC = $(wildcard ./src/*.c)
OBJS = $(patsubst %.c, %.o, $(SRC))

.PHONY: all build clean

all: clean build

build: client

./src/%.o: ./src/%.c
	$(CC)  -c $^ -o $@ $(CFLAGS)

client.o: client.c
	$(CC)  -c $^ -o $@ $(CFLAGS)

client: client.o $(OBJS)
	$(CC)  -o $@ $^ $(CFLAGS)

debug:
	$(MAKE) UI="" DEBUG="-g -D_D"

clean:
	rm -rf $(OBJS) client client.o

