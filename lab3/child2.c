#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define FILE_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <filename> <parent_pid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];
    pid_t parent_pid = atoi(argv[2]);

    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("child2: open");
        exit(EXIT_FAILURE);
    }

    char *map = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("child2: mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    char *src = map;
    char *dst = map;
    int last_was_space = 0;

    while (*src != '\0') {
        if (*src == ' ') {
            if (!last_was_space) {
                *dst++ = *src;
                last_was_space = 1;
            }
        } else {
            *dst++ = *src;
            last_was_space = 0;
        }
        src++;
    }
    *dst = '\0';

    if (msync(map, FILE_SIZE, MS_SYNC) == -1) {
        perror("child2: msync");
    }

    if (kill(parent_pid, SIGUSR1) == -1) {
        perror("child2: kill");
    }

    if (munmap(map, FILE_SIZE) == -1) {
        perror("child2: munmap");
    }
    close(fd);

    return 0;
}
