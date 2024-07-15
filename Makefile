
all: src/main.c src/celerity.c src/http.c
	gcc -Iinclude -o main src/main.c src/celerity.c src/http.c

clean:
	rm main
