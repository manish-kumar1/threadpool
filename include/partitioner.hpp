#ifndef __PARTITIONER_HPP__
#define __PARTITIONER_HPP__

#include <iterator>
#include <algorithm>

namespace thp {

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

} // namespace thp
#endif // __PARTITIONER_HPP__
