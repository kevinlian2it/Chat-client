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
    /* Read input from stdin */
    if (fgets(inbuf, BUFLEN, stdin) == NULL) {
        perror("fgets");
        return -1;
    }

    /* Check for message length */
    size_t len = strlen(inbuf);
    if (len > 0 && inbuf[len-1] != '\n') {
        fprintf(stderr, "Sorry, limit your message to 1 line of at most %d characters.\n", MAX_MSG_LEN);
        while (fgets(inbuf, BUFLEN, stdin) != NULL && inbuf[strlen(inbuf)-1] != '\n');
        return -1;
    }

    /* Trim newline character */
    inbuf[len-1] = '\0';

    /* Check for "bye" message */
    if (strcmp(inbuf, "bye") == 0) {
        strcpy(username, "");
	return 1;
    }

    /* Format message and send to server */
    snprintf(outbuf, MAX_MSG_LEN, "%.*s", MAX_MSG_LEN - MAX_NAME_LEN - 3, inbuf);
    if (send(client_socket, outbuf, strlen(outbuf)+1, 0) < 0) {
        perror("send");
        return -1;
    }

    return 0;
}
int handle_client_socket() {
    ssize_t nbytes = recv(client_socket, inbuf, BUFLEN, 0);
    if (nbytes < 0 && errno != EINTR) {
        fprintf(stderr, "Warning: Failed to receive incoming message.\n");
        return 0;
    }
    if (nbytes == 0) {
        fprintf(stderr, "\nConnection to server has been lost.\n");
        return -1;
    }
    inbuf[nbytes] = '\0';

    if (strcmp(inbuf, "bye") == 0) {
        printf("\nServer initiated shutdown.\n");
        return -1;
    } else {
        printf("%s\n", inbuf);
        return 0;
    }
}


int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server IP> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    if (inet_pton(AF_INET, argv[1], &(server_addr.sin_addr)) != 1) {
        fprintf(stderr, "Invalid IP address: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    int port;
    if (!parse_int(argv[2], &port, "port")) {
        return EXIT_FAILURE;
    }
    if (port < 1024 || port > 65535) {
        fprintf(stderr, "Invalid port number: %d\n", port);
        return EXIT_FAILURE;
    }

    char username[MAX_NAME_LEN + 1];
    
    while (1) {
        printf("Enter a username: ");
        char input[MAX_MSG_LEN + 1];
        fgets(input, MAX_MSG_LEN, stdin);

        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[--len] = '\0';
        }
	
	if (len == 0 || len > MAX_NAME_LEN) {
            fprintf(stderr, "Sorry, limit your username to %d characters.\n", MAX_NAME_LEN);
            continue;
        }
        strncpy(username, input, MAX_NAME_LEN);
        username[MAX_NAME_LEN] = '\0';
    	break;
    }

    printf("Hello, %s. Let's try to connect to the server.\n", username);
    
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Creating socket");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connecting to server");
        close(client_socket);
        return EXIT_FAILURE;
    }

    ssize_t nbytes = recv(client_socket, inbuf, BUFLEN, 0);
    if (nbytes <= 0) {
        perror("Receiving welcome message");
        close(client_socket);
        return EXIT_FAILURE;
    }
    inbuf[nbytes] = '\0';
    printf("\n%s\n\n", inbuf);

    if (send(client_socket, username, strlen(username), 0) < 0) {
        perror("Sending username");
        close(client_socket);
        return EXIT_FAILURE;
    }

    fd_set read_fds;
    int exit_flag = 0;
    while (!exit_flag) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(client_socket, &read_fds);

        int max_fd = (STDIN_FILENO > client_socket) ? STDIN_FILENO : client_socket;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            int ret = handle_stdin();
            if (ret < 0) {
                break;
            } else if (ret == 1) {
                printf("Goodbye.\n");
                exit_flag = 1;
            }
        }

        if (FD_ISSET(client_socket, &read_fds)) {
            if (handle_client_socket() < 0) {
                break;
            }
        }
    }

    close(client_socket);
    return EXIT_SUCCESS;
}
