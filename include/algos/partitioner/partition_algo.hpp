#ifndef __PARTITION_ALGO_HPP__
#define __PARTITION_ALGO_HPP__

#include <iterator>

namespace thp {
namespace partition {

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

} // namespace partition
} // namespace thp

#endif // __PARTITION_ALGO_HPP__
