#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#define SERVER_HOST "localhost"
#define SERVER_PORT "8080"

in_addr_t create_s_addr(const char *ip_addr) {
    int s_addr;
    inet_pton(AF_INET, ip_addr, &s_addr);
    return (in_addr_t) s_addr;
}

/*7
void bootstrap_server() {
    printf("Bootstrap server\n");

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("File desc of servers = %d\n", server_fd);
    if (server_fd == 0) {
        printf("Socker error\n");
        exit(1);
    }

    int opt = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        printf("Set socket opt error\n");
        exit(1);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = create_s_addr("127.0.0.1");
    address.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0) {
        printf("Bind socket error\n");
        exit(1);
    }

    if (listen(server_fd, 10)) {
        printf("Listen error\n");
        exit(1);
    }

    int buffer_size = 1024;
    char *buf = calloc(buffer_size, sizeof(char));

    while (1) {
        int client_fd = 0;

        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&address)) < 0) { 
            perror("Accept error");
            continue;
        }

        memset(buf, 0, buffer_size);
        read(client_fd, buf, buffer_size);
        printf("%s\n", buf);
        close(client_fd);
    }
}
*/

void bootstrap_server() {
    struct addrinfo hints, *server_info;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, SERVER_PORT, &hints, &server_info) != 0) {
        perror("Get address info error");
        exit(1);
    }

    int server_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    printf("File descriptor: %d\n", server_fd);
    if (server_fd < 0) {
        perror("Socket error");
        exit(1);
    }

    if (bind(server_fd, server_info->ai_addr, server_info->ai_addrlen) < 0) {
        perror("Bind error");
        exit(1);
    }

    int yes=1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof yes) < 0) {
        perror("Set socket opt error");
        exit(1);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen error");
        exit(1);
    }
    
    int client_fd;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof client_addr;

    while(1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_size);
        printf("New client with fd = %d\n", client_fd);
        if (client_fd < 0) {
            perror("Accept error");
            continue;
        }

        int recv_buf_size = 1024;
        char *recv_buf = (char *) calloc(recv_buf_size, sizeof(char));
        if (recv(client_fd, recv_buf, recv_buf_size, 0) < 0) {
            perror("Receive data error");
            continue;
        }
        printf("Receive data: %s\n", recv_buf);

        char *send_buf = "hello";
        if (send(client_fd, send_buf, strlen(send_buf), 0) < 0) {
            perror("Send data error");
            continue;
        }

        close(client_fd);
    }

    close(server_fd);
    freeaddrinfo(server_info);
}

int main() {
    bootstrap_server();

    return 0;
}
