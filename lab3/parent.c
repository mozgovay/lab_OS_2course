#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>


#define FILE_SIZE 4096
#define FILENAME "shared_data.txt"

volatile sig_atomic_t signal_received = 0;

void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        signal_received = 1;
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    int fd = open(FILENAME, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("parent: open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, FILE_SIZE) == -1) {
        perror("parent: ftruncate");
        close(fd);
        exit(EXIT_FAILURE);
    }

    char *map = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("parent: mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    const char *text = "Hello   World!  This  is  a   test.";
    strncpy(map, text, FILE_SIZE - 1);
    map[FILE_SIZE - 1] = '\0';

    if (msync(map, FILE_SIZE, MS_SYNC) == -1) {
        perror("parent: msync");
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork child1");
        exit(EXIT_FAILURE);
    }
    if (pid1 == 0) {
        char pid_str[16];
        sprintf(pid_str, "%d", getppid());
        execl("./child1", "child1", FILENAME, pid_str, NULL);
        perror("execl child1");
        exit(EXIT_FAILURE);
    }

    while (!signal_received) {
        pause();
    }
    signal_received = 0;
    waitpid(pid1, NULL, 0);

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork child2");
        exit(EXIT_FAILURE);
    }
    if (pid2 == 0) {
        char pid_str[16];
        sprintf(pid_str, "%d", getppid());
        execl("./child2", "child2", FILENAME, pid_str, NULL);
        perror("execl child2");
        exit(EXIT_FAILURE);
    }

    while (!signal_received) {
        pause();
    }
    signal_received = 0;
    waitpid(pid2, NULL, 0);

    printf("Original: Hello   World!  This  is  a   test.\n");
    printf("Result:   %s\n", map);

    if (munmap(map, FILE_SIZE) == -1) {
        perror("parent: munmap");
    }
    close(fd);

    return 0;
}
