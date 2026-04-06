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
#include <span>
#include <optional>

#define DebugAssert(x) 0

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


__attribute__((always_inline)) inline size_t
get_prefix_size(size_t total_size, size_t root_size, size_t index) {
  DebugAssert(total_size >= root_size);
  DebugAssert(index <= root_size);
  DebugAssert(0 <= index);
  DebugAssert(total_size % root_size == 0);
  size_t to_divide = total_size - root_size;
  return (to_divide * index / root_size / root_size) * root_size;
}

template <typename T>
__attribute__((always_inline)) inline void construct_root(std::span<const T> input,
                                                          std::span<T> root_output) {
  DebugAssert(root_output.size());
  DebugAssert(input.size() >= root_output.size());
  DebugAssert(input.size() % root_output.size() == 0);
  for (size_t i = 0; i < root_output.size(); ++i) {
    auto current_length = get_prefix_size(input.size(), root_output.size(), i);
    root_output[i] = input[current_length + i];
  }
}

template <typename T>
__attribute__((always_inline)) std::span<T>
get_son_subspan_in_input(std::span<T> input, size_t root_size, size_t root_index) {
  DebugAssert(input.size() >= root_size);
  DebugAssert(root_index <= root_size);
  DebugAssert(root_index > 0);
  DebugAssert(input.size() >= root_size);
  DebugAssert(input.size() % root_size == 0);
  size_t previous_length =
      get_prefix_size(input.size(), root_size, root_index - 1);
  size_t current_length = get_prefix_size(input.size(), root_size, root_index);
  return input.subspan(root_index + previous_length,
                       current_length - previous_length);
}

template <typename T>
__attribute__((always_inline)) std::span<T>
get_son_subspan_in_output(std::span<T> output, size_t root_size, size_t root_index) {
  DebugAssert(output.size() >= root_size);
  DebugAssert(root_index <= root_size);
  DebugAssert(0 < root_index);
  DebugAssert(output.size() >= root_size);
  DebugAssert(output.size() % root_size == 0);
  size_t previous_length =
      get_prefix_size(output.size(), root_size, root_index - 1);
  size_t current_length = get_prefix_size(output.size(), root_size, root_index);
  return output.subspan(root_size + previous_length,
                        current_length - previous_length);
}

__attribute__((always_inline)) size_t
get_compose_segment(size_t total_size, size_t parent_segment_number,
                    size_t son_segment_number, size_t root_size) {
  DebugAssert(parent_segment_number <= root_size);
  return (parent_segment_number == 0)
             ? 0
             : get_prefix_size(total_size, root_size,
                               parent_segment_number - 1) +
                   parent_segment_number + son_segment_number;
}

template <typename T, size_t root_size>
void build_btree(std::span<const T> input, std::span<T> output) {
  DebugAssert(input.size() == output.size());
  DebugAssert(input.size() % root_size == 0);
  if (input.size() <= root_size) {
    output.copyFrom(input);
    return;
  }
  construct_root<T>(input, output.subspan(0, root_size));
  for (size_t i = 1; i <= root_size; ++i) {
    build_btree<T, root_size>(get_son_subspan_in_input(input, root_size, i),
                              get_son_subspan_in_output(output, root_size, i));
  }
}

template <typename T>
std::optional<size_t> find_plain_segment(std::span<const T> data, T target) {
  for (size_t i = 0; i < data.size(); ++i) {
    if (data[i] == target) {
      return std::nullopt;
    } else if (data[i] > target) {
      return i;
    }
  }
  return data.size();
}

template <typename T, size_t root_size>
bool find_in_btree_iterative(std::span<const T> data, T target) {
    while (data.size() > root_size) {
        auto value = find_plain_segment(data.subspan(0, root_size), target);
        if (!value.has_value()) {
            return true;
        }
        if (value.value() == 0) {
            return false;
        }
        data = get_son_subspan_in_output(data, root_size, value.value());
    }
    return !find_plain_segment(data, target).has_value();
}

template <typename T, size_t root_size> class BTreeSearcher {
public:
  BTreeSearcher(const std::vector<T> &numbers) {
    btree_size_ = numbers.size() / root_size * root_size;
    remaining_size_ = numbers.size() - btree_size_;
    data_ = static_cast<T *>(aligned_alloc(4096, numbers.size() * sizeof(T)));
    btree_span_ = std::span<T>(data_, btree_size_);
    tail_span_ = std::span<T>(data_ + btree_size_, remaining_size_);
    build_btree<T, root_size>(std::span<const T>(numbers).subspan(0, btree_size_),
                              btree_span_);
    tail_span_.copyFrom(
        std::span<const T>(numbers).subspan(btree_size_, remaining_size_));
  }

  bool searchImpl(T query) {
    if (remaining_size_ > 0 && data_[btree_size_] <= query) {
      return !find_plain_segment(tail_span_.to_const(), query).has_value();
    }
    return !find_in_btree_iterative<T, root_size>(btree_span_.to_const(), query);
  }

  bool search(T query) override {
    if (remaining_size_ > 0 && data_[btree_size_] <= query) {
      return !find_plain_segment(tail_span_.to_const(), query).has_value();
    }
    return !find_in_btree_iterative<T, root_size>(btree_span_.to_const(), query);
  }

private:
  T *data_;
  size_t btree_size_;
  size_t remaining_size_;
  std::span<T> btree_span_;
  std::span<T> tail_span_;
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
  } else if (mode == "btree") {
    STLSearcher searcher(numbers);
    std::cerr << "Searcher built" << std::endl;
    run_benchmark(searcher, queries);
  } else {
    std::cerr << "Unknown mode: " << mode << std::endl;
    assert(false);
  }

  return 0;
}
