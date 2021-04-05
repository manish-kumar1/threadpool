#ifndef __PARTITIONER_HPP__
#define __PARTITIONER_HPP__

#include <iterator>
#include <algorithm>

namespace thp {
namespace deprecated {
// data partitioner algo class
template <typename InputIter>
struct part_algo {
  InputIter start, end;
  typename InputIter::difference_type len, step, idx, num_steps;

  explicit part_algo(InputIter s, InputIter e)
  : start{s}, end{e}, len{std::distance(s, e)}
  , step{1}, idx{0}, num_steps{0} {
     num_steps = len/step;
     if (num_steps == 0)
      num_steps = 1;
  }

  InputIter next_step() {
    if (idx + step < len) {
      idx += step;
      std::advance(start, step);
    } else {
      idx = len;
      start = end;
    }
    return start;
  }
};

template <typename InputIter, typename PartAlgo = part_algo<InputIter>>
class partitioner {
  InputIter start, cur, end;
  PartAlgo algo;

public:
  constexpr explicit partitioner(InputIter s, InputIter e, PartAlgo part)
      : start{s}, cur{s}, end{e}, algo{std::move(part)} {}

  inline constexpr decltype(auto) begin()   { return start; }
  inline constexpr decltype(auto) current() { return cur;   }
  inline constexpr decltype(auto) next()    { return cur = algo.next_step(); }
  inline constexpr decltype(auto) count()   { return algo.num_steps;         }

  constexpr inline bool operator()() { return cur == end; }
};
} // namespace deprecated

// partitioner : is a generator of views over a range, like split_view
// algo: is a strategy to generate partition views
//#if 0
template <std::input_iterator I, std::sentinel_for<I> S>
struct partition_algo {
  using iterator = I;
  using sentinel = S;

  typedef struct algo_state {
    I start;
    S end;
    explicit algo_state(I s, S e) : start(s), end(e) {}
    bool operator == (const algo_state& rhs) {
      return (start == rhs.start) && (end == rhs.end);
    }
  } state_t;

  constexpr virtual state_t next_step(const state_t& prev) = 0;
  constexpr virtual std::size_t count() const = 0;
  constexpr virtual ~partition_algo() = default;
};

template <std::input_iterator I, std::sentinel_for<I> S>
class EqualSizePartAlgo : public partition_algo<I,S> {
public:
  using state_t = typename partition_algo<I,S>::state_t;

  constexpr explicit EqualSizePartAlgo(I s, S e, std::size_t counts)
  : partition_algo<I,S>{}
  , original(s, e)
  {
    auto len = std::ranges::distance(s, e);
    counts = len <= counts ? len : counts;
    if (len%counts == 0u) {
      partition_size = len/counts;
      partition_count = counts;
    } else {
      partition_size = len/counts;
      partition_count = counts+1;
    }
    //std::cerr << "dis = " << std::ranges::distance(s, e) << std::endl;
  }

  constexpr virtual std::size_t count() const override { return partition_count; }

  constexpr virtual state_t next_step(const state_t& prev) override {
    if (prev.end == original.end)
      return state_t(original.end, original.end);

    auto dis = std::ranges::distance(prev.end, original.end);
    if (dis > partition_size)
      return state_t(prev.end, std::ranges::next(prev.end, partition_size));
    else
      return state_t(prev.end, original.end);
  }

  constexpr typename partition_algo<I, S>::state_t begin() {
    state_t beg(original.start, original.start);
    return next_step(beg);
  }

  constexpr state_t end() {
    state_t data(original.end, original.end);
    return data;
  }

  protected:
  state_t original;
  std::iter_difference_t<I> partition_size;
  std::size_t partition_count;
};

template <typename PartitionAlgoType>
class partitioner {
  std::shared_ptr<PartitionAlgoType> algo;

  class partition_iterator {
    std::shared_ptr<PartitionAlgoType> m_algo;
    typename PartitionAlgoType::state_t m_state;

    public:
    constexpr explicit partition_iterator(std::shared_ptr<PartitionAlgoType> algo, typename PartitionAlgoType::state_t state)
    : m_algo{algo}
    , m_state{state}
    {}

    partition_iterator& operator ++ (int) {
      m_state = m_algo->next_step(m_state);
      return *this;
    }
    partition_iterator& operator ++ () {
      m_state = m_algo->next_step(m_state);
      return *this;
    }

    decltype(auto) operator * () { return std::ranges::subrange(m_state.start, m_state.end); } // return original_range | views::take_while(algo->logic())

    friend bool operator == (partition_iterator a, partition_iterator b) { return a.m_state == b.m_state; }
  };

  public:
  constexpr explicit partitioner(std::shared_ptr<PartitionAlgoType> part_algo)
  : algo{std::move(part_algo)}
  {}
  inline constexpr decltype(auto) begin() { return partition_iterator(algo, algo->begin()); }
  inline constexpr decltype(auto) end() { return partition_iterator(algo, algo->end()); }
  inline constexpr std::size_t count() const { return algo->count(); }
};
//#endif

} // namespace thp
#endif // __PARTITIONER_HPP__
