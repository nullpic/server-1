all:
	gcc -Wall -g src/main.c src/game.c -o bin/server
clean:
	rm -rf src/*~
