//
// Created by s-gheldd on 3/21/16.
//
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

#define MSGLEN 3
#define BUFSIZE 6
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

union {
    enum empfaenger_state empfaenger;
    enum sender_state sender;
} state;

int main(void) {
    int exit_state;
    pid_t pid;
    pipe(pipe_one);
    pipe(pipe_two);


    if ((pid = fork()) == -1) {
        perror("fork failed");
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
    const char *fail = "\tfail";
    const char *ok = "\tok";
    char buffer[MSGLEN + BUFSIZE];
    int incoming = pipe_two[0];
    read(incoming, buffer, MSGLEN);
    switch (state.sender) {
        int ok = 0;
        case S1:
            ok = buffer[2] == '0';
            break;
        case S2:
            ok = buffer[2] == '1';
            break;
    }
    if (ok) {
        strcat(buffer, ok);
    } else {
        strcat(buffer, fail);
    }
    write_to_stdout(buffer);
}

void empfaenger_sighandler(int signum, siginfo_t *info, void *ptr) {
    const char *fail = "\tfail";
    const char *ok = "\tok";
    char buffer[MSGLEN + BUFSIZE];
    int incoming = pipe_one[0];
    read(incoming, buffer, MSGLEN);
    switch (state.empfaenger) {
        int ok = 0;
        case E1:
            ok = buffer[2] == '1';
            break;
        case E2:
            ok = buffer[2] == '2';
            break;
    }
    if (ok) {
        strcat(buffer, ok);
    } else {
        strcat(buffer, fail);
    }
    write_to_stdout(buffer);
}

int sender_run(pid_t empfaenger) {
    const char *message = "TheQuickBrownFoxOle";
    char buffer[MSGLEN];
    int outgoing = pipe_one[1];


    state.sender = S0;
    //enum sender_state state = S0;

    struct sigaction act;

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = sender_sighandler;
    act.sa_flags = SA_SIGINFO;
    if (sigaction(SIGUSR1, &act, NULL) != 0) {
        return 1;
    }


    sleep(1);

    *buffer = *(message++);
    *(buffer + 1) = CHAR_ZERO;
    *(buffer + 2) = CHAR_TERMINATOR;
    write(outgoing, buffer, MSGLEN);
    state.sender = S1;
    kill(empfaenger, SIGUSR2);

    while (*message) {
        pause();
        switch (state.sender) {
            case S1:
                *buffer = *(message++);
                *(buffer + 1) = CHAR_ONE;
                *(buffer + 2) = CHAR_TERMINATOR;
                write(outgoing, buffer, MSGLEN);
                state.sender = S2;
                break;

            case S2:
                *buffer = *(message++);
                *(buffer + 1) = CHAR_ZERO;
                *(buffer + 2) = CHAR_TERMINATOR;
                write(outgoing, buffer, MSGLEN);
                state.sender = S1;
                break;
        }
        kill(empfaenger, SIGUSR2);
    }
    pause();
    sleep(1);
    return 0;
}

int empfaenger_run() {
    const char *message = "JumpsOverTheLazyDog";
    char buffer[MSGLEN];
    int outgoing = pipe_two[1];

    pid_t sender = getppid();
    state.empfaenger = E1;

    struct sigaction act;

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = empfaenger_sighandler;
    act.sa_flags = SA_SIGINFO;
    if (sigaction(SIGUSR2, &act, NULL) != 0) {
        return 1;
    }

    while (*message) {
        pause();
        switch (state.empfaenger) {
            case E1:
                *buffer = *(message++);
                *(buffer + 1) = CHAR_ZERO;
                *(buffer + 2) = CHAR_TERMINATOR;
                write(outgoing, buffer, MSGLEN);
                state.empfaenger = E2;
                break;

            case E2:
                *buffer = *(message++);
                *(buffer + 1) = CHAR_ONE;
                *(buffer + 2) = CHAR_TERMINATOR;
                write(outgoing, buffer, MSGLEN);
                state.empfaenger = E1;
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