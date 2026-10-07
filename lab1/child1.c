#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

int main(void) {
    char buf[4096];
    ssize_t n;

    while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++)
            buf[i] = (char)toupper((unsigned char)buf[i]);

        if (write(STDOUT_FILENO, buf, n) != n) {
            perror("child1: write");
            return EXIT_FAILURE;
        }
    }
    if (n < 0) { perror("child1: read"); return EXIT_FAILURE; }
    return 0;
}
