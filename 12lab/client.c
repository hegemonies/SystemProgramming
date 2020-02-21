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
#include <unistd.h>

#define SERVER_HOST "localhost"

char *SERVER_PORT;
int COUNT_I = 0;

void start_payload(int sock_fd) {
    printf("Start payload\n");

    int buf_size = 128;
    char *buf = calloc(sizeof(char), buf_size);

    for (int i = 0; i < COUNT_I; i++) {
        sleep(i);
        sprintf(buf, "%d", i);
        printf("Send i #%d.\n", i);
        if (send(sock_fd, buf, buf_size, 0) < 0) {
            perror("Send error");
            exit(1);
        }
    }

    char *end_msg = "END";
    if (send(sock_fd, end_msg, 3, 0) < 0) {
        perror("Send error");
        exit(1);
    }
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

    start_payload(sock_fd);

    close(sock_fd);
}

void print_help() {
    printf("Help:\n");
    printf("./client -p <port> -i <count i>\n");
    printf("-p        Port of the server\n");
    printf("-i        Count i\n");
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

    while ((result = getopt(argc, argv, "p:i:")) != -1) {
        switch (result) {
        case 'p':
            SERVER_PORT = (char *) calloc(sizeof(optarg), sizeof(char));
            SERVER_PORT = optarg;
            break;
        
        case 'i':
            COUNT_I = atoi(optarg);
            break;

        default:
            error_arguments();
            break;
        }
    }

    printf("Send #%d i to localhost:%s\n", COUNT_I, SERVER_PORT);
}

int main(int argc, char *argv[]) {
    client_configure(argc, argv);
    bootstrap_client();
    printf("Finish client");
    return 0;
}
