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

bool save_file(char *filename, char *data, int data_size) {
    char *file_path = calloc(sizeof(char), strlen(PATH_STORAGE) + strlen(filename) + 2);
    strcat(file_path, PATH_STORAGE);
    strcat(file_path, "/");
    strcat(file_path, filename);
    printf("[%d] Save file to %s\n", getpid(), file_path);

    FILE *file;
    file = fopen(file_path, "w+");

    if (file == NULL) {
        logg("Error save file");
        return false;
    }

    fputs(data, file);
    fflush(file);

    fclose(file);

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
    char *recv_buf = calloc(sizeof(char), file_size + 1);

    int count_recv_data = recv(client_fd, recv_buf, file_size, 0);
    if (count_recv_data < 0) {
        perror("Receive error");
        return false;
    }

    // printf("[%d] - Count recv data: %d\n", getpid(), count_recv_data);

    *file_data = calloc(sizeof(char), count_recv_data + 2);
    memcpy(*file_data, recv_buf, count_recv_data);
    strcat(*file_data, "\0");

    return true;
}

void send_ok(int sock_fd) {
    char *ok_msg = "OK";
    if (send(sock_fd, ok_msg, strlen(ok_msg), 0) < 0) {
        perror("Send ok error");
        exit(1);
    }
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

    send_ok(client_fd);

    char *file_data;
    if (recv_file_data(client_fd, &file_data, file_size) == false) {
        send_error(client_fd, "Error file data.");
    }

    if (save_file(filename, file_data, file_size) == false) {
        send_error(client_fd, "Server error: save file error");
    }

    send_ok(client_fd);

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

void print_help() {
    printf("Help:\n");
    printf("./server -d <storage dictionary>\n\n");
    printf("-d        Path to storage dictionary\n");
}

void error_arguments() {
    fprintf(stderr, "Error argumets\n\n");
    print_help();
}

int get_socket_port(int fd) {
  struct sockaddr_in client_addr;
  socklen_t len;

  getsockname(fd, (struct sockaddr *) &client_addr, &len);

  return ntohs(client_addr.sin_port);
}

in_addr_t create_s_addr(const char *ip_addr) {
    int s_addr;
    inet_pton(AF_INET, ip_addr, &s_addr);
    return (in_addr_t) s_addr;
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

void server_configure(int argc, char *argv[]) {
    if (argc < 3) {
        error_arguments();
        exit(1);
    }

    int result = 0;

    while ((result = getopt(argc, argv, "d:")) != -1) {
        switch (result) {

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

    printf("Configuration: path of the storage = %s\n", SERVER_PORT, PATH_STORAGE);
}

int main(int argc, char *argv[]) {
    server_configure(argc, argv);
    int server_socket = bootstrap_server("127.0.0.1", 0);
    start_server(server_socket);

    return 0;
}
