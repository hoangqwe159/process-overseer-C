//
// Created by n10327622 on 12/09/2020.
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <memory.h>
#include <helpers.h>
#include <arpa/inet.h>

/* send cmd from client to server*/
void send_cmd(int, cmd_t *);

/* send flag from client to server */
void send_flag(int, flag_t *);

/**
 * main method
 * @param argc number of arguments passed from cli
 * @param argv array of arguments passed from cli
 * @return exit successful or fail
 */
int main(int argc, char **argv) {
    int sock_fd; /* socket file descriptor */
    struct sockaddr_in serverAddr; /* server address's information */
    flag_t flag_arg[3];
    cmd_t cmd_arg = {
        .flag_size =  0,
        .flag_arg =  flag_arg,
        .file_size =  0,
        .file_arg =  NULL
    }; /* store arguments and options */

    /* handle the arguments */
    handle_args(argc, argv, &cmd_arg);

    /* set up the socket */
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket\n");
        exit(EXIT_FAILURE);
    }

    /* set server's address information */
    serverAddr.sin_addr = cmd_arg.host_addr; /* server address */
    serverAddr.sin_port = htons(cmd_arg.port); /* server port */
    serverAddr.sin_family = AF_INET; /* ipv4 family */
    memset(&serverAddr.sin_zero, 0, sizeof(serverAddr.sin_zero)); /* pad 0s to sin_zero partition of the struct */

    /* connect to server */
    if (connect(sock_fd, (struct sockaddr *) &serverAddr, sizeof(struct sockaddr)) == -1) {
        fprintf(stderr, "Could not connect to overseer at %s %d\n",
                inet_ntoa(cmd_arg.host_addr), cmd_arg.port);
        exit(EXIT_FAILURE);
    }

    /* send command set to server */
    send_cmd(sock_fd, &cmd_arg);

    /* receive the response if cmd type is 2 */
    if (cmd_arg.type == cmd2) {
        char *ret = recv_str(sock_fd);
        printf("%s", ret);
        free(ret);
    }

    /* close connection and exit */
    close(sock_fd);
    exit(EXIT_SUCCESS);
}

/**
 * send command argument struct from client to server
 * @param sock_fd server socket
 * @param cmd_arg command argument
 */
void send_cmd(int sock_fd, cmd_t *cmd_arg) {
    /* send command type */

    uint32_t type = htonl(cmd_arg->type);
    if (send(sock_fd, &type, sizeof(type), 0) == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }

    /* send flags */
    /* send flag size */
    uint32_t flag_size = htonl(cmd_arg->flag_size);
    if (send(sock_fd, &flag_size, sizeof(flag_size), 0) == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }

    /* send flags arguments */
    /* send if flag exist or not */
    for (int i = 0; i < cmd_arg->flag_size; i++) {
        if (cmd_arg->flag_arg) send_flag(sock_fd, cmd_arg->flag_arg + i);
    }

    /* send file arguments */
    /* send file size */
    uint32_t file_size = htonl(cmd_arg->file_size);
    if (send(sock_fd, &file_size, sizeof(file_size), 0) == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }

    /* send file arguments */
    for (int i = 0; i < cmd_arg->file_size; i++) {
        if (!send_str(sock_fd, cmd_arg->file_arg[i])) {
            fprintf(stderr, "error sending file arguments\n");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * send flag struct from client to server
 * @param sock_fd server socket
 * @param flag_arg flag argument
 */
void send_flag(int sock_fd, flag_t *flag_arg) {
    /* send type */
    uint32_t type = htonl(flag_arg->type);
    if (send(sock_fd, &type, sizeof(type), 0) == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }

    /* send value */
    /* send if value argument exist (in case of optional argument) */
    uint16_t value_exist = flag_arg->value ? 1 : 0;
    uint16_t netLen = htons(value_exist);
    if (send(sock_fd, &netLen, sizeof(netLen), 0) == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }

    if (value_exist) {
        if (!send_str(sock_fd, flag_arg->value)) {
            fprintf(stderr, "error sending flag arguments\n");
            exit(EXIT_FAILURE);
        }
    }
}