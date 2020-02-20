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
#include <dirent.h>

#define SERVER_HOST "localhost"
#define PID_LIMIT 5

char *SERVER_PORT;

typedef int bool;
#define true 1
#define false 0

void logg(char *message) {
    if (message != NULL) {
        printf("[%d] - %s\n", getpid(), message);
    }
}

void payload(int client_fd) {
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

void pids_gc(int *count_clients) {
    if (*count_clients > PID_LIMIT) {
        logg("-- Start waiting for pids");
        while (wait(NULL) > 0);
        logg("-- Finish waiting for pids");
        *count_clients = 0;
    }
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
        } else {
            pids_gc(&count_clients);
        }
    }
}

void bootstrap_server() {
    logg("Bootstrap server");

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
    // printf("[%d] File descriptor: %d\n", getpid(), server_fd);
    if (server_fd < 0) {
        perror("Socket error");
        exit(1);
    }

    int yes=1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof yes) < 0) {
        perror("Set socket opt error");
        exit(1);
    }

    if (bind(server_fd, server_info->ai_addr, server_info->ai_addrlen) < 0) {
        perror("Bind error");
        exit(1);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen error");
        exit(1);
    }
    
    start_server(server_fd);

    close(server_fd);
    freeaddrinfo(server_info);
}

void print_help() {
    printf("Help:\n");
    printf("./server -p <port>\n");
    printf("-p        Port of the server\n");
}

void error_arguments() {
    fprintf(stderr, "Error argumets\n\n");
    print_help();
}

void server_configure(int argc, char *argv[]) {
    if (argc < 3) {
        error_arguments();
        exit(1);
    }

    int result = 0;

    while ((result = getopt(argc, argv, "p:")) != -1) {
        switch (result) {
        case 'p':
            SERVER_PORT = (char *) calloc(sizeof(optarg), sizeof(char));
            SERVER_PORT = optarg;
            break;

        default:
            error_arguments();
            break;
        }
    }

    printf("Configuration: SERVER_PORT = %s\n", SERVER_PORT);
}

int main(int argc, char *argv[]) {
    server_configure(argc, argv);
    bootstrap_server();

    return 0;
}
