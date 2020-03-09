#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <dirent.h>

#define SERVER_HOST "localhost"

typedef int bool;
#define true 1
#define false 0

void logg(char *message) {
    if (message != NULL) {
        printf("[%d] - %s\n", getpid(), message);
    }
}

in_addr_t create_s_addr(const char *ip_addr) {
    int s_addr;
    inet_pton(AF_INET, ip_addr, &s_addr);
    return (in_addr_t) s_addr;
}

void payload(void *attr) {
    int client_fd = (int)attr;

    printf("[%d] Start receive data from %d\n", getpid(), client_fd);

    int buf_size = 128;
    char *buf;

    while(1) {
        buf = calloc(sizeof(char), buf_size);
        if (recv(client_fd, buf, buf_size, 0) < 1) {
            perror("Receive data error");
            break;
        }
        if (strcmp(buf, "END") == 0) {
            printf("[%d] Stop receive data from %d\n", getpid(), client_fd);
            break;
        }
        printf("[%d] Receive from %d: %s\n", getpid(), client_fd, buf);
    }

    close(client_fd);
    exit(0);
}

void start_server(int server_fd) {
    logg("Start server");

    int client_fd;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof client_addr;

    int count_clients = 0;

    logg("Waiting for clients");

    while(1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_size);
        printf("[%d] New client with fd = %d\n", getpid(), client_fd);

        if (client_fd < 0) {
            perror("Accept error");
            continue;
        }
        count_clients++;

        pid_t pid = fork();

        if (pid < 0) {
            perror("Error fork");
        } else if (pid == 0) {
            payload(client_fd);
        }
    }
}

int get_socket_port(int fd) {
  struct sockaddr_in client_addr;
  socklen_t len;

  getsockname(fd, (struct sockaddr *) &client_addr, &len);

  return ntohs(client_addr.sin_port);
}

int bootstrap_server(char *host, int port) {
    int ret;
    struct sockaddr_in addr;
    int server_socket;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket error");
        exit(1);
    }

    int yes = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof yes) < 0) {
        perror("Set socket opt error");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = create_s_addr(host);

    if ((ret = bind(server_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))) == -1) {
        perror("Bind error");
        close(server_socket);
        exit(1);
    }

    if ((ret = listen(server_socket, 10)) == -1) {
        perror("Listen error");
        close(server_socket);
        exit(1);
    }

    printf("Server port: %d\n", get_socket_port(server_socket));
    return server_socket;
}

int main(int argc, char *argv[]) {
    int server_socket = bootstrap_server("127.0.0.1", 0);
    start_server(server_socket);

    return 0;
}
