#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

struct Clock {
  Clock(int clk_id) : clock_id(clk_id) { reset(); }

  void reset() { clock_gettime(clock_id, &start_time); }

  size_t elapsed_ns() {
    clock_gettime(clock_id, &end_time);
    return (end_time.tv_sec - start_time.tv_sec) * 1'000'000'000ull +
           (end_time.tv_nsec - start_time.tv_nsec);
  }

private:
  struct timespec start_time;
  struct timespec end_time;
  int clock_id;
};

class BinarySearcher1 {
public:
  BinarySearcher1(const std::vector<int64_t> &numbers) : numbers_(numbers) {}

  __attribute__((noinline)) bool search(int64_t query) {
    size_t left = 0;
    size_t right = numbers_.size();
    while (right - left > 1) {
      size_t mid = left + ((right - left) >> 1);
      if (numbers_[mid] <= query) {
        left = mid;
      } else {
        right = mid;
      }
    }
    return numbers_[left] == query;
  }

private:
  std::vector<int64_t> numbers_;
};

class BinarySearcher2 {
public:
  BinarySearcher2(const std::vector<int64_t> &numbers) : numbers_(numbers) {}

  __attribute__((noinline)) bool search(int64_t query) {
    const int64_t *first = numbers_.data();
    size_t len = numbers_.size();
    while (len > 0) {
      size_t half = len >> 1;
      if (first[half] < query) {
        first += half + 1;
        len -= half + 1;
      } else {
        len = half;
      }
    }
    return first != numbers_.data() + numbers_.size() && *first == query;
  }

private:
  std::vector<int64_t> numbers_;
};

class STLSearcher {
public:
  STLSearcher(const std::vector<int64_t> &numbers) : numbers_(numbers) {}

  __attribute__((noinline)) bool search(int64_t query) {
    return std::binary_search(numbers_.begin(), numbers_.end(), query);
  }

private:
  std::vector<int64_t> numbers_;
};

std::vector<int64_t> get_or_create_numbers_array(size_t n, const char *path) {
  FILE *file = fopen(path, "rb");
  if (file == nullptr) {
    std::cerr << "File " << path << " not found, generating " << n << " numbers..." << std::endl;
    std::mt19937 gen(0);
    std::uniform_int_distribution<int64_t> distrib(0, 10);
    std::vector<int64_t> numbers(n);
    int64_t current = 0;
    for (size_t i = 0; i < n; ++i) {
      current += distrib(gen);
      numbers[i] = current;
    }
    file = fopen(path, "wb");
    assert(file != nullptr);
    size_t written = fwrite(numbers.data(), sizeof(int64_t), n, file);
    assert(written == n);
    fclose(file);
    std::cerr << "Written to " << path << std::endl;
    return numbers;
  }
  std::vector<int64_t> numbers(n);
  size_t read_elements = fread(numbers.data(), sizeof(int64_t), n, file);
  if (read_elements < n) {
    std::cerr << "File has only " << read_elements << " numbers, need " << n << std::endl;
    fclose(file);
    assert(false);
  }
  fclose(file);
  return numbers;
}

template <typename Searcher>
void run_benchmark(Searcher &searcher, const std::vector<int64_t> &queries) {
  size_t answer = 0;

  // Warmup
  for (const auto &x : queries) {
    answer += searcher.search(x);
  }

  Clock cpu_clock(CLOCK_PROCESS_CPUTIME_ID);

  for (const auto &x : queries) {
    answer += searcher.search(x);
  }

  auto elapsed_ns = cpu_clock.elapsed_ns();

  std::cerr << "Answer: " << answer << std::endl;
  std::cout << std::setprecision(8)
            << elapsed_ns / static_cast<double>(queries.size()) << std::endl;
}

int main(int argc, char **argv) {
  assert(argc == 6);

  std::string mode = argv[1];
  size_t n = std::stoull(argv[2]);
  size_t queries_number = std::stoull(argv[3]);
  size_t queries_seed = std::stoull(argv[4]);
  const char *cache_file = argv[5];
  size_t maxNumber = 5 * n;

  auto numbers = get_or_create_numbers_array(n, cache_file);
  assert(numbers.size() == n);

  std::vector<int64_t> queries;
  queries.reserve(queries_number);
  std::mt19937 gen(queries_seed);
  std::uniform_int_distribution<int64_t> distrib(0, maxNumber - 1);
  for (size_t i = 0; i < queries_number; ++i) {
    queries.push_back(distrib(gen));
  }

  std::cerr << "Input generated" << std::endl;

  if (mode == "1") {
    BinarySearcher1 searcher(numbers);
    std::cerr << "Searcher built" << std::endl;
    run_benchmark(searcher, queries);
  } else if (mode == "2") {
    BinarySearcher2 searcher(numbers);
    std::cerr << "Searcher built" << std::endl;
    run_benchmark(searcher, queries);
  } else if (mode == "stl") {
    STLSearcher searcher(numbers);
    std::cerr << "Searcher built" << std::endl;
    run_benchmark(searcher, queries);
  } else {
    std::cerr << "Unknown mode: " << mode << std::endl;
    assert(false);
  }

  return 0;
}
