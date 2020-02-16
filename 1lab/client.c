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
#include "protocol.h"

#define SERVER_HOST "localhost"

char *SERVER_PORT;
char *PATH_TO_FILE;

void read_file() {

}

void send_meta_data(int sock_fd) {
    char *meta_data = "FILENAME;14151;";

    if (send(sock_fd, meta_data, strlen(meta_data), 0) < 0) {
        perror("Send file error");
        exit(1);
    }

    char *recv_buf = calloc(10, sizeof(char));
    recv(sock_fd, recv_buf, sizeof(recv_buf), 0);
    printf("Receive: %s\n", recv_buf);
}

void send_file(int sock_fd) {

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

    // char *buf = "hello";
    // if (send(sock_fd, buf, sizeof buf, 0) < 0) {
    //     perror("Send data error");
    //     exit(1);
    // }

    // int recv_buf_size = 1024;
    // char *recv_buf = (char *) calloc(recv_buf_size, sizeof(char));
    // if (recv(sock_fd, recv_buf, recv_buf_size, 0) < 0) {
    //     perror("Send data error");
    //     exit(1);
    // }
    // printf("Receive: %s.\n", recv_buf);

    read_file();
    send_meta_data(sock_fd);
    send_file(sock_fd);

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
