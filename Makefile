CC=gcc

all: src/main.c
	$(CC) -Iinclude -Wall -Wextra -Werror -o main src/main.c -g -ggdb

clean:
	rm main
