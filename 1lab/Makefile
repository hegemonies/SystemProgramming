
all: clean
	@echo "Compiling server.c"
	gcc -Wall server.c -o server -g

	@echo "Compiling client.c"
	gcc -Wall client.c -o client -g

.PHONY: server, client

server:
	rm -rf server
	@echo "Compiling server.c"
	gcc -Wall server.c -o server -g

client:
	rm -rf client
	@echo "Compiling client.c"
	gcc -Wall client.c -o client -g

clean:
	rm -rf server client
