#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 8080

in_addr_t create_s_addr(const char *ip_addr) {
    int ret, s_addr;

    ret = inet_pton(AF_INET, ip_addr, &s_addr);
    return (in_addr_t) s_addr;
}

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

    struct sockaddr_in address; #include <arpa/inet.h>
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
        int client_fd = -1;

        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&address)) < 0) { 
            printf("Accept error\n"); 
            exit(1); 
        }

        read(client_fd, buf, buffer_size);

        printf("%s\n", buf);
    }
}

int main() {
    bootstrap_server();

    return 0;
}
