//
// Created by Asus on 9/12/2020.
//

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <memory.h>
#include <netdb.h>
#include <getopt.h>
#include <helpers.h>
#include <time.h>

char current_time[TIME_BUFFER]; /* time buffer */

/**
 * print usage on error or help
 * @param msg error message to be display
 * @param type
 *  if type is error: print to stderr
 *  if type is help: print to stdout
 */
void print_usage(char *msg, enum usage type) {
    char *usage = "Usage: controller <address> <port> "
                  "{[-o out_file] [-log log_file] [-t seconds] <file> [arg...] | "
                  "mem [pid] | memkill <percent>}";

    if (type == help) {
        printf("%s\n%s\n", msg, usage);
    } else if (type == error) {
        fprintf(stderr, "%s\n%s\n", msg, usage);
    }
}

/**
 * pass command argument into the cmd_t struct based on the given
 * arguments and option flags from command line
 * @param argc number of arguments
 * @param argv array of arguments
 * @param cmd_arg command argument struct
 */
void handle_args(int argc, char **argv, cmd_t *cmd_arg) {
    struct hostent *he; /* host entry */
    uint16_t port; /* host's port */

    /* if not help there must be at least 4 arguments */
    if (argc < 4) {
        print_usage("Too few arguments", error);
        exit(EXIT_FAILURE);
    }

    /* if there is only 2 argument and it's --help */
    if (strcmp(argv[1], "--help") == 0) {
        if (argc == 2) {
            print_usage("Help Menu:", help);
            exit(EXIT_SUCCESS);
        } else {
            print_usage("Too many arguments for 'help' cmd", error);
            exit(EXIT_FAILURE);
        }
    }

    /* get the address */
    if ((he = gethostbyname(argv[1])) == NULL) {
        herror("gethostbyname\n");
        exit(EXIT_FAILURE);
    }
    cmd_arg->host_addr = *((struct in_addr *) he->h_addr);

    /* get the port from second argument */
    if (!(port = strtol(argv[2], NULL, BASE10))) {
        print_usage("Port must between 1 to 65535", error);
        exit(EXIT_FAILURE);
    }

    cmd_arg->port = port;

    /* check if third argument is mem kill or mem */
    if (strcmp(argv[3], "mem") == 0) {
        /* set up mem flag's value */
        cmd_arg->type = cmd2;
        cmd_arg->flag_arg->type = mem;
        cmd_arg->flag_arg->value = NULL;
        cmd_arg->flag_size++;

        if (argv[4]) { /* get the optional argument */
            cmd_arg->flag_arg->value = argv[4];
        }

        /* return */
        if (argc < 6) return;
        else {
            print_usage("Too many arguments for 'mem' cmd", error);
            exit(EXIT_FAILURE);
        }
    } else if (strcmp(argv[3], "memkill") == 0) {
        /* setup mem kill flag */
        cmd_arg->type = cmd3;
        cmd_arg->flag_arg->type = memkill;
        cmd_arg->flag_arg->value = NULL;
        cmd_arg->flag_size++;

        if (argv[4]) { /* get the required argument */
            cmd_arg->flag_arg->value = argv[4];
        } else {
            print_usage("Please specify percentage for memkill", error);
            exit(EXIT_FAILURE);
        }

        /* return */

        if (argc < 6) return;
        else {
            print_usage("Too many arguments for 'memkill' cmd", error);
            exit(EXIT_FAILURE);
        }
    }

    /* When we get here we know that cmd set 2 and 3 is not set, we only consider cmd set 1 */

    opterr = 0; /* disable error message for get opt in case there is argument from the executable file */
    int ch; /* character value when iterating through argv */
    bool isFlag = false; /* track if any flag in the first command group is set */
    int cmd1_args = 0; /* arguments counter for first command set to determine the position of the file */
    int oFlag = 0, lFlag = 0, tFlag = 0; /* position of flags in first command set */

    /* Executable file pointer */
    cmd_arg->file_size = 0;

    flag_t *first_arg = cmd_arg->flag_arg; /* head of flag_arg array */

    /* option string for get opt method*/
    const char *const short_options = "o:t:";
    static struct option long_options[] = {
            {"log", required_argument, NULL, 'l'},
            {NULL, 0,                  NULL, 0}
    };

    while ((ch = getopt_long_only(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (ch) {
            case 'o':
                /* create flag for output */
                cmd_arg->flag_arg->type = o;
                cmd_arg->flag_arg->value = optarg;
                cmd_arg->flag_arg++;
                cmd_arg->flag_size++;

                isFlag = true; /* set the command set 1 to true */
                oFlag = optind - 1; /* set the position of output flag */
                cmd1_args += 2; /* increment argument counter for first command set */

                /* if log flag or time flag exist before o flag return error */
                if (lFlag || tFlag) {
                    print_usage("Wrong command syntax", error);
                    exit(EXIT_FAILURE);
                }

                break;
            case 'l':
                /* create flag for log */
                cmd_arg->flag_arg->type = log;
                cmd_arg->flag_arg->value = optarg;
                cmd_arg->flag_arg++;
                cmd_arg->flag_size++;

                isFlag = true; /* set command set 1 to true */
                lFlag = optind - 1;  /* set the position of lflag */
                cmd1_args += 2; /* increment argument counter for command set 1*/

                /* if out flag is already created before its next flag is going to be log flag*/


                /* check if time flag exists or if
                 * there is anything between log flag and output flag if output flag exists*/
                if (tFlag || (oFlag && oFlag != lFlag - 2)) {
                    print_usage("Wrong command syntax", error);
                    exit(EXIT_FAILURE);
                }

                break;
            case 't':
                /* create flag for time */
                cmd_arg->flag_arg->type = t;
                cmd_arg->flag_arg->value = optarg;
                cmd_arg->flag_arg++;
                cmd_arg->flag_size++;

                isFlag = true; /* set the first command set to true */
                tFlag = optind - 1; /* store the position of the time flag */
                cmd1_args += 2; /* increment the argument counter of first command set */

                /* check if there is anything between the time flag and any other previous flags */
                if (oFlag && lFlag) { /* if output flag and log flag exist */
                    if (oFlag != tFlag - 4 && lFlag != tFlag - 2) { /* check if it's in right order */
                        print_usage("Wrong command syntax", error);
                        exit(EXIT_FAILURE);
                    }
                } else if ((oFlag && oFlag != tFlag - 2) || (lFlag && lFlag != tFlag - 2)) { /* if only output flag exist */
                    print_usage("Wrong command syntax", error);
                    exit(EXIT_FAILURE);
                }

                break;
            default:
                break;
        }
    }

    /* index of file arguments after retrieving all of the arguments */
    int file_index = cmd1_args + 3;

    /* if cmd 1 is set, flags must be in right position */
    if (isFlag && oFlag != 4 && lFlag != 4 && tFlag != 4) {
        print_usage("Wrong command syntax", error);
        exit(EXIT_FAILURE);
    }

    /* there must be file passing in the end of all option arguments */
    if (file_index == argc) {
        print_usage("Please specify file to run", error);
        exit(EXIT_FAILURE);
    }

    /* set up first command group and return */
    cmd_arg->type = cmd1;
    cmd_arg->flag_arg = first_arg;
    cmd_arg->file_size = argc - file_index;
    cmd_arg->file_arg = argv + file_index;
}

/**
 * send given string to given socket
 * @param sock_fd given socket
 * @param msg given message
 * @return
 *  true: if successfully sent
 *  false: if failed
 */
bool send_str(int sock_fd, char *msg) {
    /* send the length of the string */
    int msgLen = (int) strlen(msg) + 1;
    uint32_t netLen = htonl(msgLen);
    if (send(sock_fd, &netLen, sizeof(netLen), 0) == -1) {
        perror("send");
        return false;
    }

    /* send the message */
    if (send(sock_fd, msg, msgLen, 0) != msgLen) {
        fprintf(stderr, "send did not send all data\n");
        return false;
    }

    return true;
}

/**
 * receive string from given socket
 * @param sock_fd given socket
 * @return the string received from socket
 */
char *recv_str(int sock_fd) {
    /* get the length of the message*/
    uint32_t netLen;
    if (recv(sock_fd, &netLen, sizeof(netLen), 0) != sizeof(netLen)) {
        fprintf(stderr, "recv got invalid len value\n");
        return NULL;
    }
    int msgLen = ntohl(netLen);

    /* get the message */
    char *msg = (char *) malloc(sizeof(char) * msgLen);
    if (recv(sock_fd, msg, msgLen, 0) != msgLen) {
        fprintf(stderr, "recv got invalid message\n");
        free(msg);
        return NULL;
    }

    return msg;
}

/**
 * get current time
 * @return formatted time string
 */
char *get_time() {
    time_t timer;
    struct tm *tm_info;

    timer = time(NULL);
    tm_info = localtime(&timer);

    strftime(current_time, TIME_BUFFER, "%Y-%m-%d %H:%M:%S", tm_info);

    return current_time;
}