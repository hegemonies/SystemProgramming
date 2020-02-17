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
char *PATH_STORAGE;
struct dirent *pDirent;
DIR *pDir;

typedef int bool;
#define true 1
#define false 0

void logg(char *message) {
    if (message != NULL) {
        printf("[%d] - %s\n", getpid(), message);
    }
}

bool save_file(char *filename, char *data) {
    // TODO: save filename with data to PATH_STORAGE
    return true;
}

bool recv_filename_and_size_from(int client_fd, char **filename, int *file_size) {
    int recv_buf_size = 1024;
    char *recv_buf = (char *) calloc(recv_buf_size, sizeof(char));
    if (recv(client_fd, recv_buf, recv_buf_size, 0) < 0) {
        perror("Receive data error");
        return false;
    }

    char *lexem = strtok(recv_buf, ";");
    // printf("Get lexem1: %s\n", lexem);
    if (lexem == NULL) {
        logg("First lexem is null");
        return false;
    }

    *filename = calloc(sizeof(char), strlen(lexem));
    memcpy(*filename, lexem, strlen(lexem));
    // printf("Filename: %s\n", *filename);

    lexem = strtok(NULL, ";");
    // printf("Get lexem2: %s\n", lexem);
    if (lexem == NULL) {
        logg("Second lexem is null");
        return false;
    }

    *file_size = atoi(lexem);
    // printf("File size: %d\n", *file_size);

    return true;
}

void send_error(int client_fd, char *error_msg) {
    if (error_msg != NULL) {
        char *f_msg = "Send error: ";
        char *result_msg = calloc(sizeof(char), strlen(f_msg) + strlen(error_msg));
        strcat(result_msg, f_msg);
        strcat(result_msg, error_msg);
        logg(result_msg);
        send(client_fd, error_msg, strlen(error_msg), 0);
    } else {
        char *msg = "Server error.";
        send(client_fd, msg, strlen(msg), 0);
    }

    close(client_fd);
    exit(1);
}

bool recv_file_data(int client_fd, char **file_data, int file_size) {
    char *recv_buf = calloc(sizeof(char), file_size);

    int count_recv_data = recv(client_fd, recv_buf, file_size, 0);
    if (count_recv_data < 0) {
        perror("Receive error");
        return false;
    }

    *file_data = calloc(sizeof(char), count_recv_data);
    memcpy(file_data, recv_buf, count_recv_data);
    return true;
}

void payload(int client_fd) {
    // printf("Payload on proc with pid #%d\n", getpid());
    logg("Start payload");

    // Receive meta data with info about size and name of file
    char *filename;
    int file_size;

    if (recv_filename_and_size_from(client_fd, &filename, &file_size) == false) {
        send_error(client_fd, "Error meta data in request.");
    }

    printf("[%d] - Receive meta data: %s;%d;\n", getpid(), filename, file_size);

    char *ok_msg = "OK";
    send(client_fd, ok_msg, strlen(ok_msg), 0);

    char *file_data;
    if (recv_file_data(client_fd, &file_data, file_size) == false) {
        send_error(client_fd, "Error file data.");
    }
    logg("Receive file data ok");

    if (save_file(filename, file_data) == false) {
        send_error(client_fd, "Server error: save file error");
    }

    send(client_fd, ok_msg, strlen(ok_msg), 0);
    
    logg("Save file ok");

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
    // printf("Start server on pid #%d\n", getpid());
    logg("Start server");

    int client_fd;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof client_addr;

    int count_clients = 0;

    while(1) {
        logg("Waiting for clients");
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
    printf("[%d] File descriptor: %d\n", getpid(), server_fd);
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
