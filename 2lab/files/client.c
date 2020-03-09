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
#include <sys/sendfile.h>

char *SERVER_HOST;
char *SERVER_PORT;
char *PATH_TO_FILE;
FILE *file;
int filesize;
char *file_data;

void send_filesize(int sock_fd, int filesize) {
    char sfile_size[(int)((ceil(log10(filesize))+1)*sizeof(char))];
    sprintf(sfile_size, "%d", filesize);
    if (send(sock_fd, sfile_size, strlen(sfile_size), 0) < 0) {
        perror("Send file error");
        exit(1);
    }
}

void send_file(int sock_fd) {
    if (send(sock_fd, file_data, filesize, 0) < 0) {
        perror("Error send file");
        exit(1);
    }
    // int file_fd = fileno(file);
    // printf("file_fd=%d\n", file_fd);
    // if (sendfile(sock_fd, file_fd, NULL, filesize) < 0) {
    //     perror("Error sendfile");
    //     exit(1);
    // }
    printf("File has been send successfully\n");
}

int bootstrap_client() {
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

    return sock_fd;
}

void print_help() {
    printf("Help:\n");
    printf("./client -h <host> -p <port> -f <file>\n\n");
    printf("-h        Host of the server\n");
    printf("-p        Port of the server\n");
    printf("-f        Path to the file\n");
}

void error_arguments() {
    fprintf(stderr, "Error argumets\n\n");
    print_help();
}

void client_configure(int argc, char *argv[]) {
    if (argc < 7) {
        error_arguments();
        exit(1);
    }

    int result = 0;

    while ((result = getopt(argc, argv, "h:p:f:")) != -1) {
        switch (result) {
        case 'h':
            SERVER_HOST = (char *) calloc(sizeof(optarg), sizeof(char));
            SERVER_HOST = optarg;
            break;

        case 'p':
            SERVER_PORT = (char *) calloc(sizeof(optarg), sizeof(char));
            SERVER_PORT = optarg;
            break;

        case 'f':
            PATH_TO_FILE = (char *) calloc(sizeof(optarg), sizeof(char));
            PATH_TO_FILE = optarg;

            file = fopen(PATH_TO_FILE, "r+");
            if (file == NULL) {
                printf("Error read file: %s\n", PATH_TO_FILE);
                exit(1);
            }

            fseek(file, 0, SEEK_END);
            filesize = ftell(file);
            printf("Filesize = %d\n", filesize);
            rewind(file);

            file_data = calloc(sizeof(char), filesize + 1);

            int read_size = fread(file_data, sizeof(char), filesize, file);
            printf("Readsize = %d\n", read_size);

            if (filesize != read_size) {
                fclose(file);
                printf("Error2 read file: %s\n", PATH_TO_FILE);
                exit(1);
            }

            fclose(file);

            break;

        default:
            error_arguments();
            break;
        }
    }

    printf("Send %s to localhost:%s\n", PATH_TO_FILE, SERVER_PORT);
}

void payload(int sock_fd) {
    send_filesize(sock_fd, filesize);

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

    send_file(sock_fd);

    memset(buf, 0, buf_size);
    // printf("waiting for response from server\n");
    if (recv(sock_fd, buf, buf_size, 0) < 0) {
        perror("Receive response error");
        exit(1);
    }

    // printf("Receive response: %s\n", buf);

    if (strcmp(buf, "OK") != 0) {
        printf("Server error\n");
        exit(1);
    }

    printf("Send file ok\n");

    close(sock_fd);
}

int main(int argc, char *argv[]) {
    client_configure(argc, argv);
    int sock_fd = bootstrap_client();
    payload(sock_fd);
    printf("Finish client");
    return 0;
}
