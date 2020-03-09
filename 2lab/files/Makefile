
all: clean server client

.PHONY: server, client

server:
	rm -rf server
	@echo "Compiling server.c"
	gcc --std=c99 -pthread -Wall server.c -o myserver -O3 -g

client:
	rm -rf client
	@echo "Compiling client.c"
	gcc -Wall client.c -o client -O3 -g -lm

clean:
	rm -rf myserver client
