#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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
    if (fd == -1) { perror("child1: shm_open"); exit(EXIT_FAILURE); }

    char *map = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) { perror("child1: mmap"); close(fd); exit(EXIT_FAILURE); }

    for (int i = 0; map[i] != '\0'; i++) {
        map[i] = toupper((unsigned char)map[i]);
    }

    if (msync(map, SHM_SIZE, MS_SYNC) == -1) perror("child1: msync");
    if (kill(parent_pid, SIGUSR1) == -1) perror("child1: kill");
    if (munmap(map, SHM_SIZE) == -1) perror("child1: munmap");
    close(fd);
    return 0;
}
