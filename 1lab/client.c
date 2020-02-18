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
#include <math.h>
#include <sys/wait.h>

#define SERVER_HOST "localhost"

char *SERVER_PORT;
char *PATH_TO_FILE;

void read_file(char **file_data, int *file_size) {
    FILE *file;
    file = fopen(PATH_TO_FILE, "r+");

    if (file == NULL) {
        printf("Error read file: %s\n", PATH_TO_FILE);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    printf("Filesize = %d\n", *file_size);
    rewind(file);

    *file_data = calloc(sizeof(char), *file_size + 1);

    int read_size = fread(*file_data, sizeof(char), *file_size, file);
    printf("Readsize = %d\n", read_size);
    // *file_data[*file_size - 1] = '\0';

    if (*file_size != read_size) {
        fclose(file);
        printf("Error2 read file: %s\n", PATH_TO_FILE);
        exit(1);
    }

    fclose(file);
}

void get_filename_from_path(char **filename) {
    if (strstr(PATH_TO_FILE, "/") != NULL) {
        strtok(PATH_TO_FILE, "/");
        char *buf;
        while ((buf = strtok(NULL, "/")) != NULL) {
            *filename = calloc(sizeof(char), strlen(buf));
            memcpy(*filename, buf, strlen(buf));
        }
    } else {
        *filename = calloc(sizeof(char), strlen(PATH_TO_FILE));
        memcpy(*filename, PATH_TO_FILE, strlen(PATH_TO_FILE));
    }

    printf("Filename: %s\n", *filename);
}

void send_meta_data(int sock_fd, char *meta_data) {
    if (send(sock_fd, meta_data, strlen(meta_data), 0) < 0) {
        perror("Send file error");
        exit(1);
    }

    // char *recv_buf = calloc(10, sizeof(char));
    // recv(sock_fd, recv_buf, sizeof(recv_buf), 0);
    // printf("Receive: %s\n", recv_buf);
}

void create_meta_data(char **meta_data, char *filename, int file_size) {
    char sfile_size[(int)((ceil(log10(file_size))+1)*sizeof(char))];
    // itoa(file_size, sfile_size, 10);
    sprintf(sfile_size, "%d", file_size);

    *meta_data = calloc(sizeof(char), strlen(filename) + strlen(sfile_size) + 3);
    strcat(*meta_data, filename);
    strcat(*meta_data, ";");
    strcat(*meta_data, sfile_size);
    strcat(*meta_data, ";");
    strcat(*meta_data, "\0");
}

void send_file(int sock_fd, char *file_data, int file_size) {
    if (send(sock_fd, file_data, file_size, 0) < 0) {
        perror("Error send file");
        exit(1);
    }
    printf("File has been send successfully\n");
}

void bootstrap_client() {
    printf("Bootstrap client");
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

    char *filename;
    get_filename_from_path(&filename);
    
    char *file_data;
    int file_size;
    read_file(&file_data, &file_size);

    char *meta_data; // todo: add concat filename and filesize
    create_meta_data(&meta_data, filename, file_size);
    printf("Metadata: %s\n", meta_data);

    send_meta_data(sock_fd, meta_data);

    int buf_size = 16;
    char *buf = calloc(sizeof(char), buf_size);
    if (recv(sock_fd, buf, buf_size, 0) < 0) {
        perror("Receive response error");
        exit(1);
    }

    printf("Receive response: %s\n", buf);

    if (strcmp(buf, "OK") != 0) {
        printf("Server error\n");
        exit(1);
    }

    send_file(sock_fd, file_data, file_size);

    memset(buf, 0, buf_size);
    printf("waiting for response from server\n");
    if (recv(sock_fd, buf, buf_size, 0) < 0) {
        perror("Receive response error");
        exit(1);
    }

    printf("Receive response: %s\n", buf);

    if (strcmp(buf, "OK") != 0) {
        printf("Server error\n");
        exit(1);
    }

    close(sock_fd);
}

void print_help() {
    printf("Help:\n");
    printf("./client -p <port> -f <file>\n\n");
    printf("-p        Port of the server\n");
    printf("-f        Path to file\n");
}

void error_arguments() {
    fprintf(stderr, "Error argumets\n\n");
    print_help();
}

void client_configure(int argc, char *argv[]) {
    if (argc < 5) {
        error_arguments();
        exit(1);
    }

    int result = 0;

    while ((result = getopt(argc, argv, "p:f:")) != -1) {
        switch (result) {
        case 'p':
            SERVER_PORT = (char *) calloc(sizeof(optarg), sizeof(char));
            SERVER_PORT = optarg;
            break;

        case 'f':
            PATH_TO_FILE = (char *) calloc(sizeof(optarg), sizeof(char));
            PATH_TO_FILE = optarg;
            break;

        default:
            error_arguments();
            break;
        }
    }

    printf("Send %s to localhost:%s\n", PATH_TO_FILE, SERVER_PORT);
}

int main(int argc, char *argv[]) {
    client_configure(argc, argv);
    bootstrap_client();
    printf("Finish client");
    return 0;
}
