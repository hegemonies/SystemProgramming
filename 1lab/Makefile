
all:
	rm -rf client
	@echo "Compiling client.c"
	gcc -Wall client.c -o client -O3 -g -lm
	rm -rf server
	@echo "Compiling server.c"
	gcc -Wall server.c -o server -O3 -g

.PHONY: server, client

server:
	rm -rf server
	@echo "Compiling server.c"
	gcc -Wall server.c -o server -O3 -g

client:
	rm -rf client
	@echo "Compiling client.c"
	gcc -Wall client.c -o client -O3 -g -lm

clean:
	rm -rf server client
