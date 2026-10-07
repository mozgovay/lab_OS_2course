#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int main(void) {
    char in[4096];
    char out[4096];
    ssize_t n;
    int prev_space = 0;

    while ((n = read(STDIN_FILENO, in, sizeof(in))) > 0) {
        ssize_t k = 0;
        for (ssize_t i = 0; i < n; i++) {
            if (in[i] == ' ') {
                if (prev_space) continue;
                prev_space = 1;
            } else {
                prev_space = 0;
            }
            out[k++] = in[i];
        }
        if (write(STDOUT_FILENO, out, k) != k) {
            perror("child2: write");
            return EXIT_FAILURE;
        }
    }
    if (n < 0) { perror("child2: read"); return EXIT_FAILURE; }
    return 0;
}
