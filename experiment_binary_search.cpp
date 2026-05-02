#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <immintrin.h>
#include <iomanip>
#include <iostream>
#include <new>
#include <optional>
#include <random>
#include <span>
#include <vector>

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
    std::ranges::copy(input, output.begin());
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
        for (size_t i = 1; i <= root_size; ++i) {
          __builtin_prefetch(
              get_son_subspan_in_output(data, root_size, i).data(), 0, 3);
        }
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
    std::ranges::copy(
        std::span<const T>(numbers).subspan(btree_size_, remaining_size_),
        tail_span_.begin());
  }

  __attribute__((noinline)) bool search(T query) {
    if (remaining_size_ > 0 && data_[btree_size_] <= query) {
      return !find_plain_segment(std::span<const T>(tail_span_), query).has_value();
    }
    return !find_in_btree_iterative<T, root_size>(std::span<const T>(btree_span_), query);
  }

private:
  T *data_;
  size_t btree_size_;
  size_t remaining_size_;
  std::span<T> btree_span_;
  std::span<T> tail_span_;
};

class BinarySearcher_btree_avx {
public:
  static constexpr auto HDIS = std::hardware_destructive_interference_size;
  static constexpr auto ITEM_SIZE = HDIS / sizeof(int64_t);
  static constexpr auto MM512I_PER_ITEM =
      (ITEM_SIZE * sizeof(int64_t)) / sizeof(__m512i);
  static constexpr size_t NODE_ARITY = 8;
  using Item = std::array<int64_t, ITEM_SIZE>;
  struct alignas(64) IndexNode {
    std::array<int64_t, NODE_ARITY> pivots;
  };

  __attribute__((target("avx512f")))
  BinarySearcher_btree_avx(const std::vector<int64_t> &numbers)
      : leaves_(nullptr), leaf_count_(0), leaf_capacity_(0), nodes_(nullptr),
        internal_count_(0), leaf_depth_(0), last_internal_level_offset_(0),
        min_value_(0), max_value_(-1) {
    static_assert(sizeof(IndexNode) == 64);

    if (numbers.empty()) {
      return;
    }

    min_value_ = numbers.front();
    max_value_ = numbers.back();

    leaf_count_ = (numbers.size() + ITEM_SIZE - 1) / ITEM_SIZE;
    leaf_capacity_ = 1;
    while (leaf_capacity_ < leaf_count_) {
      leaf_capacity_ *= NODE_ARITY;
      ++leaf_depth_;
    }

    leaves_ = static_cast<Item *>(
        std::aligned_alloc(HDIS, sizeof(Item) * leaf_capacity_));
    assert(leaves_ != nullptr);

    for (size_t leaf = 0; leaf < leaf_count_; ++leaf) {
      const size_t start = leaf * ITEM_SIZE;
      const size_t copy_count = std::min(ITEM_SIZE, numbers.size() - start);
      for (size_t i = 0; i < copy_count; ++i) {
        leaves_[leaf][i] = numbers[start + i];
      }
      for (size_t i = copy_count; i < ITEM_SIZE; ++i) {
        leaves_[leaf][i] = max_value_;
      }
    }
    for (size_t leaf = leaf_count_; leaf < leaf_capacity_; ++leaf) {
      leaves_[leaf].fill(max_value_);
    }

    if (leaf_capacity_ > 1) {
      internal_count_ = (leaf_capacity_ - 1) / (NODE_ARITY - 1);
      nodes_ = static_cast<IndexNode *>(
          std::aligned_alloc(HDIS, sizeof(IndexNode) * internal_count_));
      assert(nodes_ != nullptr);

      if (leaf_depth_ > 0) {
        size_t level_nodes = 1;
        for (size_t level = 1; level < leaf_depth_; ++level) {
          last_internal_level_offset_ += level_nodes;
          level_nodes *= NODE_ARITY;
        }
      }

      build_tree();
    }
  }

  ~BinarySearcher_btree_avx() {
    std::free(nodes_);
    std::free(leaves_);
  }

  BinarySearcher_btree_avx(const BinarySearcher_btree_avx &) = delete;
  BinarySearcher_btree_avx &
  operator=(const BinarySearcher_btree_avx &) = delete;
  BinarySearcher_btree_avx(BinarySearcher_btree_avx &&) = delete;
  BinarySearcher_btree_avx &operator=(BinarySearcher_btree_avx &&) = delete;

  __attribute__((always_inline)) __attribute__((target("avx512f"))) bool
  small_search(const Item *item, int64_t query) const {
    const __m512i cmp = _mm512_set1_epi64(query);

    if constexpr (ITEM_SIZE == 8) {
      const __m512i chunk = _mm512_load_si512((const void *)item);
      return _mm512_cmpeq_epi64_mask(chunk, cmp) != 0;
    } else {
      const int64_t *p = (const int64_t *)item;
      for (size_t i = 0; i < MM512I_PER_ITEM; ++i) {
        const __m512i chunk = _mm512_load_si512((const void *)(p + i * 8));
        if (_mm512_cmpeq_epi64_mask(chunk, cmp) != 0) {
          return true;
        }
      }
      for (size_t i = MM512I_PER_ITEM * 8; i < ITEM_SIZE; ++i) {
        if (p[i] == query) {
          return true;
        }
      }
      return false;
    }
  }

  __attribute__((noinline)) __attribute__((target("avx512f"))) bool
  search(int64_t query) {
    if (leaves_ == nullptr || query < min_value_ || query > max_value_) {
      return false;
    }
    if (leaf_capacity_ == 1) {
      return small_search(leaves_, query);
    }

    const __m512i qvec = _mm512_set1_epi64(query);
    uint32_t node_index = 0;
    for (size_t level = 0; level + 1 < leaf_depth_; ++level) {
      const IndexNode &node = nodes_[node_index];
      const __m512i pivots = _mm512_load_si512((const void *)&node);
      const uint32_t mask = _mm512_cmp_epi64_mask(pivots, qvec, _MM_CMPINT_LT);
      const uint32_t child = __builtin_popcount(mask);
      node_index = node_index * NODE_ARITY + 1 + child;
      if (level + 2 < leaf_depth_) {
        __builtin_prefetch(nodes_ + node_index * NODE_ARITY + 1, 0, 1);
      }
    }

    const IndexNode &node = nodes_[node_index];
    const __m512i pivots = _mm512_load_si512((const void *)&node);
    const uint32_t mask = _mm512_cmp_epi64_mask(pivots, qvec, _MM_CMPINT_LT);
    const uint32_t child = __builtin_popcount(mask);
    const uint32_t leaf_index =
        (node_index - static_cast<uint32_t>(last_internal_level_offset_)) *
            NODE_ARITY +
        child;
    return small_search(leaves_ + leaf_index, query);
  }

private:
  __attribute__((target("avx512f"))) void build_tree() {
    std::vector<int64_t> current_maxima(leaf_capacity_);
    for (size_t i = 0; i < leaf_capacity_; ++i) {
      current_maxima[i] = leaves_[i][ITEM_SIZE - 1];
    }

    size_t level_count = leaf_capacity_ / NODE_ARITY;
    size_t level_offset = last_internal_level_offset_;

    while (true) {
      std::vector<int64_t> parent_maxima(level_count);
      for (size_t node = 0; node < level_count; ++node) {
        IndexNode &index_node = nodes_[level_offset + node];
        for (size_t child = 0; child < NODE_ARITY; ++child) {
          const int64_t child_max = current_maxima[node * NODE_ARITY + child];
          index_node.pivots[child] = child_max;
        }
        parent_maxima[node] = index_node.pivots[NODE_ARITY - 1];
      }
      if (level_offset == 0) {
        break;
      }
      current_maxima.swap(parent_maxima);
      level_count /= NODE_ARITY;
      level_offset = (level_offset - 1) / NODE_ARITY;
    }
  }

  Item *leaves_;
  size_t leaf_count_;
  size_t leaf_capacity_;
  IndexNode *nodes_;
  size_t internal_count_;
  size_t leaf_depth_;
  size_t last_internal_level_offset_;
  int64_t min_value_;
  int64_t max_value_;
};

std::vector<int64_t> get_or_create_numbers_array(size_t n, const char *path) {
  FILE *file = fopen(path, "rb");
  if (file == nullptr) {
    std::cerr << "File " << path << " not found, generating " << n
              << " numbers..." << std::endl;
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
    std::cerr << "File has only " << read_elements << " numbers, need " << n
              << std::endl;
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
    BTreeSearcher<int64_t, 8> searcher(numbers);
    std::cerr << "Searcher built" << std::endl;
    run_benchmark(searcher, queries);
  } else if (mode == "btree_avx") {
    BinarySearcher_btree_avx searcher(numbers);
    std::cerr << "Searcher built" << std::endl;
    run_benchmark(searcher, queries);
  } else {
    std::cerr << "Unknown mode: " << mode << std::endl;
    assert(false);
  }

  return 0;
}
