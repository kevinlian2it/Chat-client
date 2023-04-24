#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "util.h"

int client_socket = -1;
char username[MAX_NAME_LEN + 1];
char inbuf[BUFLEN + 1];
char outbuf[MAX_MSG_LEN + 1];

int handle_stdin() {
    fgets(inbuf, BUFLEN, stdin);
    if (strncmp(inbuf, "/quit", 5) == 0) {
        return -1;
    }
    snprintf(outbuf, MAX_MSG_LEN, "%s: %s", username, inbuf);
    send(client_socket, outbuf, strlen(outbuf), 0);
    return 0;

}

int handle_client_socket() {
    int bytes_received = recv(client_socket, inbuf, BUFLEN, 0);
    if (bytes_received <= 0) {
        return -1;
    }
    inbuf[bytes_received] = '\0';
    printf("%s", inbuf);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Usage: %s <server IP> <server port> <username>\n", argv[0]);
        return 1;
    }

    strncpy(username, argv[3], MAX_NAME_LEN);
    username[MAX_NAME_LEN] = '\0';

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP");
        return 2;
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket");
        return 3;
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 4;
    }

    set_nonblocking(client_socket);
    set_nonblocking(STDIN_FILENO);

    printf("Connected to the chat server!\nType '/quit' to disconnect.\n");

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(client_socket, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        if (select(client_socket + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(client_socket, &read_fds)) {
            if (handle_client_socket() < 0) {
                break;
            }
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (handle_stdin() < 0) {
                break;
            }
        }
    }

    close(client_socket);
    printf("Disconnected from the chat server.\n");
    return 0;
}
