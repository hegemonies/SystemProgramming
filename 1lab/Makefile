
all: server client
	# @echo "Compiling server.c"
	# gcc -Wall server.c -o server -g

	# @echo "Compiling client.c"
	# gcc -Wall client.c -o client -g

.PHONY: server, client

server: server.c
	rm -rf server
	@echo "Compiling server.c"
	gcc -Wall server.c -o server -O3 -g

client: client.c
	rm -rf client
	@echo "Compiling client.c"
	gcc -Wall client.c -o client -O3 -g -lm

clean:
	rm -rf server client
