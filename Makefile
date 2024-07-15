
all: src/main.c src/celerity.c src/http.c
	gcc -Iinclude -Wall -Wextra -Werror -o main src/main.c src/celerity.c src/http.c

clean:
	rm main
