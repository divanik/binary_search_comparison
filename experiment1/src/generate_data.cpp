#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

int main(int argc, char **argv) {
    assert(argc == 6);

    size_t n = std::stoull(argv[1]);
    size_t queries_number = std::stoull(argv[2]);
    size_t queries_seed = std::stoull(argv[3]);
    const char *data_file = argv[4];
    const char *queries_file = argv[5];

    std::mt19937 gen(0);
    std::uniform_int_distribution<int64_t> step_distrib(0, 10);
    std::vector<int64_t> numbers(n);
    int64_t current = 0;
    for (size_t i = 0; i < n; ++i) {
        current += step_distrib(gen);
        numbers[i] = current;
    }

    FILE *file = fopen(data_file, "wb");
    assert(file != nullptr);
    size_t written = fwrite(numbers.data(), sizeof(int64_t), n, file);
    assert(written == n);
    fclose(file);
    std::cerr << "Written " << n << " numbers to " << data_file << std::endl;

    std::mt19937 qgen(queries_seed);
    std::uniform_int_distribution<int64_t> query_distrib(0, (int64_t)(numbers.back()));
    std::vector<int64_t> queries(queries_number);
    for (size_t i = 0; i < queries_number; ++i) {
        queries[i] = query_distrib(qgen);
    }

    FILE *qfile = fopen(queries_file, "wb");
    assert(qfile != nullptr);
    size_t qwritten = fwrite(queries.data(), sizeof(int64_t), queries_number, qfile);
    assert(qwritten == queries_number);
    fclose(qfile);
    std::cerr << "Written " << queries_number << " queries to " << queries_file << std::endl;

    return 0;
}
