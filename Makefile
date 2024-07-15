
all: src/main.c src/celerity.c
	gcc -Iinclude -Wall -Wextra -Werror -o main src/main.c src/celerity.c

clean:
	rm main
