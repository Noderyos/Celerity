
all: src/main.c src/celerity.c src/http.c src/route.c
	gcc -Iinclude -Wall -Wextra -Werror -o main src/main.c src/celerity.c src/http.c src/route.c -g -ggdb

clean:
	rm main
