#ifndef __EQUALSIZE_PARTALGO__
#define __EQUALSIZE_PARTALGO__

#include <iterator>
#include <algorithm>

#include "include/algos/partitioner/partition_algo.hpp"

namespace thp {
namespace partition {

template <std::input_iterator I, std::sentinel_for<I> S>
class EqualSize : public partition_algo<I,S> {
public:
  using state_t = typename partition_algo<I,S>::state_t;

  constexpr explicit EqualSize(I s, S e, std::size_t counts)
  : partition_algo<I,S>{}
  , original(s, e)
  {
    auto len = std::ranges::distance(s, e);
    counts = len <= counts ? len : counts;
    partition_size = len/counts;
    partition_count = counts;

    if (len%counts != 0u) {
      partition_count = counts+1;
    }
  }

  constexpr virtual std::size_t count() const override { return partition_count; }

  constexpr virtual state_t next_step(const state_t& prev) override {
    if (prev.end == original.end)
      return state_t(original.end, original.end);

    auto dis = std::ranges::distance(prev.end, original.end);
    auto new_end = dis > partition_size ? std::ranges::next(prev.end, partition_size) : original.end;
    return state_t(prev.end, new_end);
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

} // namespace partition
} // namespace thp

#endif // __EQUALSIZ_PARTALGO__
