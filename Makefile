CC = gcc
CFLAGS = -Wall -Iinclude
SRC = src/main.c src/server.c src/http.c src/mime.c src/files.c

all:
	$(CC) $(CFLAGS) $(SRC) -o minihttpd

clean:
	rm -f minihttpd
