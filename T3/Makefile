CC=gcc
CFLAGS=-I.

client: client.c buffer.c helpers.c parson.c
	$(CC) -o client client.c helpers.c parson.c buffer.c -Wall

run: client
	./client

clean:
	rm -f *.o client
