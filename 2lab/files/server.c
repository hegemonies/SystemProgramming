#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
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
#define THREAD_LIMIT 50

char *SERVER_PORT;
char *PATH_STORAGE;
FILE *file;
char *filename = "buf_file.txt";

typedef int bool;
#define true 1
#define false 0

pthread_mutex_t mutex;

void logg(char *message) {
    pthread_mutex_lock(&mutex);
    if (message != NULL) {
        printf("[%d] - %s\n", getpid(), message);
    }
    pthread_mutex_unlock(&mutex);
}

void thread_gc(int threads[]) {
    printf("[%d] -- GC start", getpid());
    for (int i = 0; i < THREAD_LIMIT; i++) {
        printf("[%d] -- Join of %d thread", getpid(), threads[i]);
        pthread_join(threads[i], NULL);
    }
    printf("[%d] -- GC finish", getpid());
}

void save_file(char *filename, char *data, int data_size) {
    pthread_mutex_lock(&mutex);
    fputs(data, file);
    // fclose(file);
    pthread_mutex_unlock(&mutex);
}

bool recv_file_size(int client_fd, int *file_size) {
    int recv_buf_size = 1024;
    char *recv_buf = (char *) calloc(recv_buf_size, sizeof(char));

    if (recv(client_fd, recv_buf, recv_buf_size, 0) < 0) {
        perror("Receive data error");
        return false;
    }

    *file_size = atoi(recv_buf);

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

void *payload(void *arg) {
    int client_fd = (int)arg;
    logg("Start payload");

    int file_size;

    if (recv_file_size(client_fd, &file_size) == false) {
        send_error(client_fd, "Error filesize in request.");
    }

    printf("[%d] - Receive filesize: %d;\n", getpid(), file_size);

    printf("Send ok to client %d\n", client_fd);
    send_ok(client_fd);

    char *file_data;
    logg("Waiting for receive file data");
    if (recv_file_data(client_fd, &file_data, file_size) == false) {
        send_error(client_fd, "Error file data.");
    }

    logg("Save file");
    save_file(filename, file_data, file_size);

    printf("Send ok to client %d\n", client_fd);
    send_ok(client_fd);

    close(client_fd);
    pthread_exit(0);
}

void start_server(int server_fd) {
    logg("Start server");

    int client_fd;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof client_addr;

    int count_clients = 0;

    logg("Waiting for clients");

    int threads[THREAD_LIMIT];

    if (pthread_mutex_init(&mutex, NULL) != 0) {
        perror("Mutex init error");
        exit(1);
    }

    while(1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_size);
        printf("[%d] New client with fd = %d\n", getpid(), client_fd);
        if (client_fd < 0) {
            perror("Accept error");
            continue;
        }
        count_clients++;

        pthread_t thread;
        if (pthread_create(&thread, NULL, payload, (void *)client_fd) != 0) {
            perror("Error pthread_create");
        }
        threads[count_clients] = thread;
        if (count_clients > THREAD_LIMIT - 2) {
            thread_gc(threads);
            count_clients = 0;
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
    char *file_path;

    while ((result = getopt(argc, argv, "d:")) != -1) {
        switch (result) {

        case 'd':
            PATH_STORAGE = (char *) calloc(sizeof(optarg), sizeof(char));
            PATH_STORAGE = optarg;

            file_path = calloc(sizeof(char), strlen(PATH_STORAGE) + strlen(filename) + 2);
            strcat(file_path, PATH_STORAGE);
            strcat(file_path, "/");
            strcat(file_path, filename);
            strcat(file_path, "\0");

            file = fopen(file_path, "a+");
            if (file == NULL) {
                fprintf(stderr, "Cannot open file '%s'\n", file_path);
                exit(1);
            }

            break;

        default:
            error_arguments();
            break;
        }
    }

    printf("Configuration: path of the file = %s\n", file_path);
}

int main(int argc, char *argv[]) {
    server_configure(argc, argv);
    int server_socket = bootstrap_server("127.0.0.1", 0);
    start_server(server_socket);

    return 0;
}
