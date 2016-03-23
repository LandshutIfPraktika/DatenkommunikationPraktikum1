//
// Created by s-gheldd on 3/21/16.
//
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define BUFFSIZE 3
#define CHAR_ZERO '0'
#define CHAR_ONE '1'
#define CHAR_TERMINATOR '\0'

enum sender_state {
    S0, S1, S2
};

enum empfaenger_state {
    E1, E2
};

void sender_sighandler(int signum, siginfo_t *info, void *ptr);

void empfaenger_sighandler(int signum, siginfo_t *info, void *ptr);

int sender_run(pid_t empfaenger);

int empfaenger_run();

void write_to_stdout(char *string);

int pipe_one[2], pipe_two[2];

int main(void) {
    int exit_state;
    pid_t pid;
    pipe(pipe_one);
    pipe(pipe_two);


    if ((pid = fork()) == -1) {
        exit(1);
    }


    if (pid) {
        //parent
        close(pipe_one[0]);
        close(pipe_two[1]);
        exit_state = sender_run(pid);
        close(pipe_one[1]);
        close(pipe_two[0]);
    } else {
        //child
        close(pipe_one[1]);
        close(pipe_two[0]);
        exit_state = empfaenger_run();
        close(pipe_one[0]);
        close(pipe_two[1]);
    }
    return exit_state;
}

void sender_sighandler(int signum, siginfo_t *info, void *ptr) {
    char buffer[BUFFSIZE];
    int incoming = pipe_two[0];
    read(incoming, buffer, BUFFSIZE);
    write_to_stdout(buffer);
}

void empfaenger_sighandler(int signum, siginfo_t *info, void *ptr) {
    char buffer[BUFFSIZE];
    int incoming = pipe_one[0];
    read(incoming, buffer, BUFFSIZE);
    write_to_stdout(buffer);
}

int sender_run(pid_t empfaenger) {
    const char *message = "TheQuickBrownFoxOle";
    char buffer[BUFFSIZE];
    int outgoing = pipe_one[1];

    enum sender_state state = S0;

    struct sigaction act;

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = sender_sighandler;
    act.sa_flags = SA_SIGINFO;
    if (sigaction(SIGUSR1, &act, NULL) != 0) {
        return 1;
    }


    sleep(4);

    *buffer = *(message++);
    *(buffer + 1) = CHAR_ZERO;
    *(buffer + 2) = CHAR_TERMINATOR;
    write(outgoing, buffer, BUFFSIZE);
    state = S1;
    kill(empfaenger, SIGUSR2);

    while (*message) {
        pause();
        switch (state) {
            case S1:
                *buffer = *(message++);
                *(buffer + 1) = CHAR_ONE;
                *(buffer + 2) = CHAR_TERMINATOR;
                write(outgoing, buffer, BUFFSIZE);
                state = S2;
                break;

            case S2:
                *buffer = *(message++);
                *(buffer + 1) = CHAR_ZERO;
                *(buffer + 2) = CHAR_TERMINATOR;
                write(outgoing, buffer, BUFFSIZE);
                state = S1;
                break;
        }
        kill(empfaenger, SIGUSR2);
    }
    pause();
    sleep(2);
    return 0;
}

int empfaenger_run() {
    const char *message = "JumpsOverTheLazyDog";
    char buffer[BUFFSIZE];
    int outgoing = pipe_two[1];

    pid_t sender = getppid();
    enum empfaenger_state state = E1;

    struct sigaction act;

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = empfaenger_sighandler;
    act.sa_flags = SA_SIGINFO;
    if (sigaction(SIGUSR2, &act, NULL) != 0) {
        return 1;
    }

    while (*message) {
        pause();
        switch (state) {
            case E1:
                *buffer = *(message++);
                *(buffer + 1) = CHAR_ZERO;
                *(buffer + 2) = CHAR_TERMINATOR;
                write(outgoing, buffer, BUFFSIZE);
                state = E2;
                break;

            case E2:
                *buffer = *(message++);
                *(buffer + 1) = CHAR_ONE;
                *(buffer + 2) = CHAR_TERMINATOR;
                write(outgoing, buffer, BUFFSIZE);
                state = E1;
                break;
        }
        kill(sender, SIGUSR1);
    }
    pause();
    kill(sender, SIGUSR1);
    sleep(1);
    return 0;
}

void write_to_stdout(char *string) {
    write(STDOUT_FILENO, string, strlen(string));
    write(STDOUT_FILENO, "\n", 1);
}