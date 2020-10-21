//
// Created by duchi on 9/29/2020.
//
#define _GNU_SOURCE

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <memory.h>
#include <errno.h>
#include <signal.h>
#include <helpers.h>
#include <wait.h>

pid_t pid; /* pid to store the pid of the children */

/**
 * signal handler
 * @param sig received signal
 * @param siginfo signal info
 * @param context stack context
 */
void handler(int sig, siginfo_t *siginfo, void *context) {
    if (sig == SIGINT) {
        kill(pid, SIGKILL);
        sleep(1);
        int status;
        waitpid(pid, &status, 0);
        printf("%s - %d has terminated with status code %d\n",
               get_time(), pid, WEXITSTATUS(status));
        _exit(EXIT_SUCCESS);
    }
}

/**
 * main function
 * @param argc number of arguments from cli
 * @param argv array of arguments from cli
 * @return exit successfully or failed
 */
int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    /* exec and termination time out */
    struct timespec exec_timeout = {
            .tv_sec = strtol(argv[1], NULL, BASE10),
            .tv_nsec = 0
    }, term_timeout = {
            .tv_sec = strtol(argv[2], NULL, BASE10),
            .tv_nsec = 0
    };

    /* open logfile and outfile if exist */
    int outFile = 0, logFile = 0;
    if (strcmp(argv[3], "") != 0) {
        if ((outFile = open(argv[3], O_CREAT | O_TRUNC | O_WRONLY, 0644)) == -1) {
            perror("open outfile");
        }
    }

    if (strcmp(argv[4], "") != 0) {
        if ((logFile = open(argv[4], O_CREAT | O_TRUNC | O_WRONLY, 0644)) == -1) {
            perror("open logfile");
        }
    }

    /* shift the argument variable to the start of the file executable */
    argv += 5;
    argc -= 5;

    /* concat file arguments into string */
    char file_args[256];
    strcpy(file_args, argv[0]);

    for (int i = 1; i < argc; i++) {
        strcat(file_args, " ");
        strcat(file_args, argv[i]);
    }

    /* fork info */
    int status, result;

    /* pipe info to signal the parent of successful executed child */
    int buf, n;
    int pipe_fds[2];
    pipe2(pipe_fds, O_CLOEXEC);
    bool executed = false;

    pid = fork();
    if (pid == -1) {
        perror("fork");
    } else if (pid == 0) { /* child */
        /* set pgid so that sigint doesn't interrupt the child */
        setpgid(0, 0);

        /* ignore SIGINT */
        signal(SIGINT, SIG_IGN);

        /* duplicate outfile descriptor onto stdout and stdin if exist */
        if (outFile) {
            dup2(outFile, STDOUT_FILENO);
            dup2(outFile, STDERR_FILENO);
        }

        /* close the reading side of the pipe*/
        close(pipe_fds[0]);

        /* execute the file */
        execv(argv[0], argv);

        /* write to the pipe the error code */
        write(pipe_fds[1], &errno, sizeof(errno));

        /* send error code if exec failed */
        _exit(errno);
    } else { /* parent */

        /* add int, usr1 handler */
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_sigaction = handler;
        sa.sa_flags = SA_SIGINFO;
        sigaction(SIGINT, &sa, NULL);

        /* set of signal to wait for */
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGCHLD);

        /* block child signal in case child terminated  and send signal before
         * sleeping and being consumed by sigtimedwait */
        sigprocmask(SIG_BLOCK, &set, NULL);

        /* log management to logfile if exist */
        if (logFile)
            dup2(logFile, STDOUT_FILENO);

        /* inform of file execution */
        printf("%s - attempting to execute %s\n", get_time(), file_args);

        /* get the pipe's data to know if there's any error occurred with execv */
        close(pipe_fds[1]);
        n = read(pipe_fds[0], &buf, sizeof(buf));
        if (n == 0) { /* nothing in pipe -> successfully executed */
            /* change executed to true */
            executed = true;

            /* sleep for 1 second before informing of a success execution */
            sleep(1);

            /* inform of successful execution */
            printf("%s - %s has been executed with pid %d\n", get_time(), file_args, pid);

            /* pause the parent until either child terminated or timeout */
            sigtimedwait(&set, NULL, &exec_timeout);
        } else {
            printf("%s - could not execute %s - Error: %s\n",
                   get_time(), file_args, strerror(buf));
        }
        close(pipe_fds[0]);

        /* In here, the code will be continue by either timout or child signal.

        * We need to use waitpid to get the status of the child to see if it exited or not*/

        result = waitpid(pid, &status, WNOHANG);
        if (result == 0) { /* timeout, child is still running */
            printf("%s - sent SIGTERM to %d\n", get_time(), pid);
            kill(pid, SIGTERM);

            /* wait for child signal until timeout */
            sigtimedwait(&set, NULL, &term_timeout);

            result = waitpid(pid, &status, WNOHANG);
            if (result == 0) { /* timeout, child is still running  */
                printf("%s - sent SIGKILL to %d\n", get_time(), pid);
                kill(pid, SIGKILL);

                /* final check */
                result = waitpid(pid, &status, 0);
                if (result < 0) {
                    perror("waitpid");
                } else {
                    printf("%s - %d has terminated with status code %d\n", get_time(), pid,
                           WEXITSTATUS(status));
                }
            } else if (result > 0) { /* child has finished*/
                printf("%s - %d has terminated with status code %d\n",
                       get_time(), pid, WEXITSTATUS(status));
            } else {
                perror("waitpid");
            }
        } else if (result > 0) { /* child has finished */
            // printf("Result: %d\n", result);
            if (executed) {

                printf("%s - %d has terminated with status code %d\n",
                       get_time(), pid, WEXITSTATUS(status));
            }
        } else {
            perror("waitpid");
        }

        close(logFile);

        exit(EXIT_SUCCESS);
    }
}