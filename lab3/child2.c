#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <errno.h>

#define SHM_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <shm_name> <parent_pid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char *shm_name = argv[1];
    pid_t parent_pid = atoi(argv[2]);

    int fd = shm_open(shm_name, O_RDWR, 0);
    if (fd == -1) { perror("child2: shm_open"); exit(EXIT_FAILURE); }

    char *map = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) { perror("child2: mmap"); close(fd); exit(EXIT_FAILURE); }

    char *src = map, *dst = map;
    int last_was_space = 0;
    while (*src != '\0') {
        if (*src == ' ') {
            if (!last_was_space) { *dst++ = *src; last_was_space = 1; }
        } else {
            *dst++ = *src;
            last_was_space = 0;
        }
        src++;
    }
    *dst = '\0';

    if (msync(map, SHM_SIZE, MS_SYNC) == -1) perror("child2: msync");
    if (kill(parent_pid, SIGUSR1) == -1) perror("child2: kill");
    if (munmap(map, SHM_SIZE) == -1) perror("child2: munmap");
    close(fd);
    return 0;
}
