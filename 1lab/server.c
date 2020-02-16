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
#include "protocol.h"

#define SERVER_HOST "localhost"
#define PID_LIMIT 5

char *SERVER_PORT;
char *PATH_STORAGE;
struct dirent *pDirent;
DIR *pDir;

void save_file(char *filename, char *data) {
    // TODO: save filename with data to PATH_STORAGE
}

void get_filename_and_size_from(char *recv_buf, char *filename, int *file_size) {
    char *lexem = strtok(recv_buf, ";");
    printf("Get lexem: %s\n", lexem);

    if (lexem == NULL) {
        filename = NULL;
        file_size = NULL;
        return;
    }

    filename = malloc(sizeof(char) * strlen(lexem));
    memcpy(filename, lexem, strlen(lexem));
    printf("Filename: %s\n", filename);

    lexem = strtok(NULL, ";");
    printf("Get lexem: %s\n", lexem);

    if (lexem == NULL) {
        filename = NULL;
        file_size = NULL;
        return;
    }

    file_size = malloc(sizeof(int) * 1);
    *file_size = atoi(lexem);
    printf("File size: %d\n", *file_size);
}

void payload(int client_fd) {
    printf("Payload on proc with pid #%d\n", getpid());

    // Receive meta data with info about size and name of file
    int recv_buf_size = 1024;
    char *recv_buf = (char *) calloc(recv_buf_size, sizeof(char));
    if (recv(client_fd, recv_buf, recv_buf_size, 0) < 0) {
        perror("Receive data error");
        return;
    }
    // printf("Receive data: %s.\n", recv_buf);
    char *filename;
    int *file_size;
    get_filename_and_size_from(recv_buf, filename, file_size);

    if (filename == NULL || file_size == NULL) {
        char *error_msg = "Error meta data in request.";
        send(client_fd, error_msg, strlen(error_msg), 0);
        return;
    }

    printf(" Receive meta data: %s %d\n", filename, *file_size);

    char *ok_msg = "OK";
    send(client_fd, ok_msg, strlen(ok_msg), 0);

    close(client_fd);
    exit(0);
}

void pids_gc(int *count_clients) {
    if (*count_clients > PID_LIMIT) {
        printf("Start waiting for pids\n");
        while (wait(NULL) > 0);
        printf("Finish waiting for pids\n");
        *count_clients = 0;
    }
}

void start_server(int server_fd) {
    printf("Start server on pid #%d\n", getpid());

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
            payload(client_fd);
        } else {
            pids_gc(&count_clients);
        }
    }
}

void bootstrap_server() {
    printf("Bootstrap server");
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
    
    start_server(server_fd);

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

void server_configure(int argc, char *argv[]) {
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

            pDir = opendir(PATH_STORAGE);
            if (pDir == NULL) {
                fprintf(stderr, "Cannot open directory '%s'\n", PATH_STORAGE);
                exit(1);
            }

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
