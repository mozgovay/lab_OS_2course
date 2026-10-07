#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

static void die(const char *msg) { perror(msg); exit(EXIT_FAILURE); }

static void close_all(int *p1, int *p2, int *p3) {
    int *arr[3] = { p1, p2, p3 };
    for (int a = 0; a < 3; a++)
        for (int i = 0; i < 2; i++)
            if (arr[a][i] > 2) close(arr[a][i]);
}

static pid_t spawn(const char *path, int in_fd, int out_fd,
                   int *p1, int *p2, int *p3) {
    pid_t pid = fork();
    if (pid < 0) die("fork");

    if (pid == 0) {
        if (dup2(in_fd,  STDIN_FILENO)  < 0) die("dup2 in");
        if (dup2(out_fd, STDOUT_FILENO) < 0) die("dup2 out");
        close_all(p1, p2, p3);
        execl(path, path, NULL);
        perror("execl"); _exit(EXIT_FAILURE);
    }
    return pid;
}

int main(void) {
    int p1[2], p2[2], p3[2];
    if (pipe(p1) < 0) die("pipe1");
    if (pipe(p2) < 0) die("pipe2");
    if (pipe(p3) < 0) die("pipe3");

    pid_t c1 = spawn("./child1", p1[0], p2[1], p1, p2, p3);
    pid_t c2 = spawn("./child2", p2[0], p3[1], p1, p2, p3);

    close(p1[0]); close(p2[0]); close(p2[1]); close(p3[1]);
    int wfd = p1[1], rfd = p3[0];

    printf("Enter lines (empty line to finish):\n");

    char *line = NULL;
    size_t cap = 0;
    ssize_t len;

    while ((len = getline(&line, &cap, stdin)) > 0) {
        if (len == 1 && line[0] == '\n') break;
        if (write(wfd, line, len) != len) die("write to pipe1");

        char out[4096];
        ssize_t got = 0;
        while (got < (ssize_t)sizeof(out) - 1) {
            ssize_t r = read(rfd, out + got, 1);
            if (r < 0) die("read from pipe3");
            if (r == 0) break;
            if (out[got++] == '\n') break;
        }
        if (got > 0) {
            out[got] = '\0';
            printf("Result: %s", out);
        }
    }
    free(line);
    close(wfd);

    waitpid(c1, NULL, 0);
    waitpid(c2, NULL, 0);
    return 0;
}
