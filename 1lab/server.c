#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#define SERVER_HOST "localhost"

char *SERVER_PORT;
char *PATH_STORAGE;

in_addr_t create_s_addr(const char *ip_addr) {
    int s_addr;
    inet_pton(AF_INET, ip_addr, &s_addr);
    return (in_addr_t) s_addr;
}

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

    int count_clients = 0;

    while(1) {
        printf("Waiting for clients\n");
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_size);
        printf("New client with fd = %d\n", client_fd);
        if (client_fd < 0) {
            perror("Accept error");
            continue;
        }
        count_clients++;

        pid_t pid = fork();

        if (pid < 0) {
            perror("Error fork");
        } else if (pid == 0) {
            printf("Payload...");
            int recv_buf_size = 1024;
            char *recv_buf = (char *) calloc(recv_buf_size, sizeof(char));
            if (recv(client_fd, recv_buf, recv_buf_size, 0) < 0) {
                perror("Receive data error");
                continue;
            }
            printf("Receive data: %s.\n", recv_buf);

            char *send_buf = "hello";
            if (send(client_fd, send_buf, strlen(send_buf), 0) < 0) {
                perror("Send data error");
                continue;
            }

            close(client_fd);
            exit(0);
        } else {
            if (count_clients > 9) {
                wait(NULL);
            }
        }
    }

    close(server_fd);
    freeaddrinfo(server_info);
}

void print_help() {
    printf("Help:\n");
    printf("./server -p <port> -d <storage dictionary>\n\n");
    printf("-p        Port of the server\n");
    printf("-d        Path to storage dictionary\n");
}

void error_arguments() {
    fprintf(stderr, "Error argumets\n\n");
    print_help();
}

void server_configuration(int argc, char *argv[]) {
    if (argc < 5) {
        error_arguments();
        exit(1);
    }

    int result = 0;

    while ((result = getopt(argc, argv, "p:d:")) != -1) {
        switch (result) {
        case 'p':
            SERVER_PORT = (char *) calloc(sizeof(optarg), sizeof(char));
            SERVER_PORT = optarg;
            break;

        case 'd':
            PATH_STORAGE = (char *) calloc(sizeof(optarg), sizeof(char));
            PATH_STORAGE = optarg;
            break;

        default:
            error_arguments();
            break;
        }
    }

    printf("Configuration ok: SERVER_PORT = %s; PATH_STORAGE = %s\n", SERVER_PORT, PATH_STORAGE);
}

int main(int argc, char *argv[]) {
    server_configure(argc, argv);
    bootstrap_server();

    return 0;
}
