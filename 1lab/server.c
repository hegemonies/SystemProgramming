#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_PORT 8080

int main() {
    printf("Bootstrap server\n");

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("File desc of servers = %d\n", server_fd);
    if (server_fd == 0) {
        printf("Socker error");
        exit(1);
    }

    int opt = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        printf("Set socket opt error");
        exit(1);
    }

    struct sockaddr_in address; 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, &address, (struct sockaddr *)&address, (socklen_t*)&address) < 0) {

    }

    // bind(server_fd, address, );
}
