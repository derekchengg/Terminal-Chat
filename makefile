CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror

all: build

build:
	gcc $(CFLAGS) simpletalk.c list.o -o s-talk -lpthread 

run: build
	./s-talk

valgrind: build
	valgrind --leak-check=full

clean:
	rm -f s-talk