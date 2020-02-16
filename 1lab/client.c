#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#define SERVER_HOST "localhost"
#define SERVER_PORT "8080"

in_addr_t create_s_addr(const char *ip_addr) {
    int s_addr;
    inet_pton(AF_INET, ip_addr, &s_addr);
    return (in_addr_t) s_addr;
}
/*
void bootstrap_client() {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("File desc of client = %d\n", socket_fd);
    if (socket_fd == 0) {
        perror("Socker error");
    }
    
	struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = create_s_addr("127.0.0.1");
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		printf("Connect Failed\n");
		exit(1);
	}

	char buffer[1024];
	memset(buffer, '0', sizeof(buffer));

    for (int i = 0; i < 5; i++) {
        sprintf(buffer,  "%d", i);
        write(socket_fd, buffer, sizeof(buffer));
    }

    close(socket_fd);
}
*/

void bootstrap_client() {
    struct addrinfo hints, *res;
    int sock_fd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(SERVER_HOST, SERVER_PORT, &hints, &res);

    sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    printf("Socket fd = %d\n", sock_fd);

    if (connect(sock_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Connect error");
    }

    char *buf = "hello";
    if (send(sock_fd, buf, sizeof buf, 0) < 0) {
        perror("Send data error");
        exit(1);
    }

    int recv_buf_size = 1024;
    char *recv_buf = (char *) calloc(recv_buf_size, sizeof(char));
    if (recv(sock_fd, recv_buf, recv_buf_size, 0) < 0) {
        perror("Send data error");
        exit(1);
    }

    close(sock_fd);
}

int main() {
    bootstrap_client();
    printf("Finish client");
    return 0;
}
