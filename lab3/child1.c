#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
        perror("child1: open");
        exit(EXIT_FAILURE);
    }

    char *map = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("child1: mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    int len = strlen(map);
    for (int i = 0; i < len / 2; i++) {
        char tmp = map[i];
        map[i] = map[len - 1 - i];
        map[len - 1 - i] = tmp;
    }

    if (msync(map, FILE_SIZE, MS_SYNC) == -1) {
        perror("child1: msync");
    }

    if (kill(parent_pid, SIGUSR1) == -1) {
        perror("child1: kill");
    }

    if (munmap(map, FILE_SIZE) == -1) {
        perror("child1: munmap");
    }
    close(fd);

    return 0;
}
