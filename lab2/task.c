#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#define OP_ERODE  0
#define OP_DILATE 1

typedef struct {
    double *in;
    double *out;
    int rows;
    int cols;
    int w;
    int r0;
    int r1;
    int op;
} Task;

static void erode_region(double *in, double *out, int rows, int cols, int w, int r0, int r1) {
    int half = w / 2;
    for (int i = r0; i < r1; i++) {
        for (int j = 0; j < cols; j++) {
            double mn = 1e300;
            for (int di = -half; di <= half; di++) {
                int ii = i + di;
                if (ii < 0) ii = 0;
                if (ii >= rows) ii = rows - 1;
                for (int dj = -half; dj <= half; dj++) {
                    int jj = j + dj;
                    if (jj < 0) jj = 0;
                    if (jj >= cols) jj = cols - 1;
                    double v = in[ii * cols + jj];
                    if (v < mn) mn = v;
                }
            }
            out[i * cols + j] = mn;
        }
    }
}

static void dilate_region(double *in, double *out, int rows, int cols, int w, int r0, int r1) {
    int half = w / 2;
    for (int i = r0; i < r1; i++) {
        for (int j = 0; j < cols; j++) {
            double mx = -1e300;
            for (int di = -half; di <= half; di++) {
                int ii = i + di;
                if (ii < 0) ii = 0;
                if (ii >= rows) ii = rows - 1;
                for (int dj = -half; dj <= half; dj++) {
                    int jj = j + dj;
                    if (jj < 0) jj = 0;
                    if (jj >= cols) jj = cols - 1;
                    double v = in[ii * cols + jj];
                    if (v > mx) mx = v;
                }
            }
            out[i * cols + j] = mx;
        }
    }
}

static void *worker(void *arg) {
    Task *t = (Task *)arg;
    printf("[thread] id=%lu rows=[%d,%d) op=%s\n",
           (unsigned long)pthread_self(), t->r0, t->r1,
           t->op == OP_ERODE ? "erode" : "dilate");
    fflush(stdout);

    if (t->op == OP_ERODE)
        erode_region(t->in, t->out, t->rows, t->cols, t->w, t->r0, t->r1);
    else
        dilate_region(t->in, t->out, t->rows, t->cols, t->w, t->r0, t->r1);
    return NULL;
}

static void run_parallel_op(double *in, double *out, int rows, int cols, int w,
                            int nthreads, int op,
                            pthread_t *threads, Task *tasks) {
    int base = rows / nthreads;
    int rem  = rows % nthreads;
    int start = 0;

    for (int k = 0; k < nthreads; k++) {
        int len = base + (k < rem ? 1 : 0);
        tasks[k].in   = in;
        tasks[k].out  = out;
        tasks[k].rows = rows;
        tasks[k].cols = cols;
        tasks[k].w    = w;
        tasks[k].r0   = start;
        tasks[k].r1   = start + len;
        tasks[k].op   = op;
        start += len;

        pthread_create(&threads[k], NULL, worker, &tasks[k]);
    }

    for (int k = 0; k < nthreads; k++) {
        pthread_join(threads[k], NULL);
    }
}

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static double algorithm_sequential(double *a, double *b, int rows, int cols, int w, int k) {
    double t0 = now_ms();

    for (int i = 0; i < k; i++) {
        erode_region(a, b, rows, cols, w, 0, rows);
        double *tmp = a; a = b; b = tmp;
    }

    for (int i = 0; i < k; i++) {
        dilate_region(a, b, rows, cols, w, 0, rows);
        double *tmp = a; a = b; b = tmp;
    }

    return now_ms() - t0;
}

static double algorithm_parallel(double *a, double *b, int rows, int cols, int w, int k, int nthreads) {
    pthread_t *threads = malloc(sizeof(pthread_t) * nthreads);
    Task *tasks = malloc(sizeof(Task) * nthreads);

    double t0 = now_ms();

    for (int i = 0; i < k; i++) {
        run_parallel_op(a, b, rows, cols, w, nthreads, OP_ERODE, threads, tasks);
        double *tmp = a; a = b; b = tmp;
    }

    for (int i = 0; i < k; i++) {
        run_parallel_op(a, b, rows, cols, w, nthreads, OP_DILATE, threads, tasks);
        double *tmp = a; a = b; b = tmp;
    }

    double dt = now_ms() - t0;

    free(threads);
    free(tasks);
    return dt;
}

int main(int argc, char **argv) {
    int rows = 1000, cols = 1000, w = 3, k = 3, nthreads = 4, reps = 3, demo = 0;

    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "-n") == 0 && i + 1 < argc) rows = cols = atoi(argv[++i]);
        else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) w = atoi(argv[++i]);
        else if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) k = atoi(argv[++i]);
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) nthreads = atoi(argv[++i]);
        else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) reps = atoi(argv[++i]);
        else if (strcmp(argv[i], "-d") == 0) demo = 1;
    }

    if (w % 2 == 0) w++;
    if (w < 1) w = 1;
    if (nthreads < 1) nthreads = 1;
    if (reps < 1) reps = 1;

    size_t sz = (size_t)rows * cols;
    double *a  = malloc(sz * sizeof(double));
    double *b  = malloc(sz * sizeof(double));
    double *a2 = malloc(sz * sizeof(double));
    double *b2 = malloc(sz * sizeof(double));

    srand(12345);
    for (size_t i = 0; i < sz; i++) {
        a[i] = (double)rand() / RAND_MAX * 255.0;
    }
    memcpy(a2, a, sz * sizeof(double));

    if (demo) {
        printf("=== DEMO ===\n");
        algorithm_parallel(a2, b2, rows, cols, w, 1, nthreads);
        printf("=== DONE ===\n");
        free(a); free(b); free(a2); free(b2);
        return 0;
    }

    double ts_min = 1e300;
    for (int r = 0; r < reps; r++) {
        double t = algorithm_sequential(a, b, rows, cols, w, k);
        if (t < ts_min) ts_min = t;
    }

    double tp_min = 1e300;
    for (int r = 0; r < reps; r++) {
        memcpy(a2, a, sz * sizeof(double));
        double t = algorithm_parallel(a2, b2, rows, cols, w, k, nthreads);
        if (t < tp_min) tp_min = t;
    }

    int ok = 1;
    for (size_t i = 0; i < sz; i++) {
        if (fabs(a[i] - a2[i]) > 1e-9) { ok = 0; break; }
    }

    double speedup    = ts_min / tp_min;
    double efficiency = speedup / nthreads;

    printf("size=%d w=%d k=%d threads=%d reps=%d\n", rows, w, k, nthreads, reps);
    printf("sequential_ms=%.3f\n", ts_min);
    printf("parallel_ms=%.3f\n", tp_min);
    printf("speedup=%.4f\n", speedup);
    printf("efficiency=%.4f\n", efficiency);
    printf("match=%s\n", ok ? "yes" : "no");

    free(a); free(b); free(a2); free(b2);
    return 0;
}
