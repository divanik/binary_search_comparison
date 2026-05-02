#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


size_t lower_bound1(const int64_t *numbers, size_t n, int64_t query);
size_t lower_bound2(const int64_t *numbers, size_t n, int64_t query);

typedef struct {
    struct timespec start_time;
    struct timespec end_time;
    int clock_id;
} Clock;

static Clock clock_new(int clk_id) {
    Clock c;
    c.clock_id = clk_id;
    clock_gettime(clk_id, &c.start_time);
    return c;
}

static size_t clock_elapsed_ns(Clock *c) {
    clock_gettime(c->clock_id, &c->end_time);
    return (size_t)(c->end_time.tv_sec - c->start_time.tv_sec) * 1000000000ull +
           (size_t)(c->end_time.tv_nsec - c->start_time.tv_nsec);
}

static size_t lower_bound_check(const int64_t *numbers, size_t n, int64_t query) {
    size_t left = 0;
    size_t right = n;
    while (left < right) {
        size_t mid = left + (right - left) / 2;
        if (numbers[mid] < query)
            left = mid + 1;
        else
            right = mid;
    }
    return left;
}

static int64_t *read_file(const char *path, size_t *out_n) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Cannot open file: %s\n", path);
        assert(0);
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    assert(file_size >= 0 && file_size % (long)sizeof(int64_t) == 0);
    size_t n = (size_t)file_size / sizeof(int64_t);
    int64_t *data = (int64_t *)malloc(n * sizeof(int64_t));
    assert(data != NULL);
    size_t read = fread(data, sizeof(int64_t), n, file);
    assert(read == n);
    fclose(file);
    *out_n = n;
    return data;
}

#define RUN_BENCHMARK(search_fn) do { \
    size_t answer = 0; \
    for (size_t i = 0; i < queries_n; ++i) { \
        size_t got = search_fn(numbers, n, queries[i]); \
        assert(got == lower_bound_check(numbers, n, queries[i])); \
        answer += got; \
    } \
    Clock cpu_clock = clock_new(CLOCK_PROCESS_CPUTIME_ID); \
    for (size_t i = 0; i < queries_n; ++i) \
        answer += search_fn(numbers, n, queries[i]); \
    size_t elapsed_ns = clock_elapsed_ns(&cpu_clock); \
    printf("%.8f\n", elapsed_ns / (double)queries_n); \
} while (0)


#define RUN_BENCHMARK_PERF(search_fn) do { \
    size_t answer = 0; \
    for (size_t i = 0; i < queries_n; ++i) { \
        size_t got = search_fn(numbers, n, queries[i]); \
        assert(got == lower_bound_check(numbers, n, queries[i])); \
        answer += got; \
    } \
    fprintf(stderr, "PID: %d\n", getpid()); \
    sleep(30); \
    Clock cpu_clock = clock_new(CLOCK_PROCESS_CPUTIME_ID); \
    for (size_t i = 0; i < queries_n; ++i) \
        answer += search_fn(numbers, n, queries[i]); \
    size_t elapsed_ns = clock_elapsed_ns(&cpu_clock); \
    printf("%.8f\n", elapsed_ns / (double)queries_n); \
} while (0)

int main(int argc, char **argv) {
    assert(argc == 4 || argc == 5);

    const char *mode = argv[1];
    const char *data_file = argv[2];
    const char *queries_file = argv[3];

    int perf = 0;
    if (argc == 5) {
        perf = 1;
    }

    size_t n, queries_n;
    int64_t *numbers = read_file(data_file, &n);
    int64_t *queries = read_file(queries_file, &queries_n);

    if (perf) {
        if (strcmp(mode, "1") == 0) {
            RUN_BENCHMARK_PERF(lower_bound1);
        } else if (strcmp(mode, "2") == 0) {
            RUN_BENCHMARK_PERF(lower_bound2);
        } else {
            fprintf(stderr, "Unknown mode: %s\n", mode);
            assert(0);
        }
    } else {
        if (strcmp(mode, "1") == 0) {
            RUN_BENCHMARK(lower_bound1);
        } else if (strcmp(mode, "2") == 0) {
            RUN_BENCHMARK(lower_bound2);
        } else {
            fprintf(stderr, "Unknown mode: %s\n", mode);
            assert(0);
        }
    }
    free(numbers);
    free(queries);
    return 0;
}
