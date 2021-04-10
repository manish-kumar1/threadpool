#ifndef __PARTITIONER_HPP__
#define __PARTITIONER_HPP__

#include <iterator>
#include <algorithm>
#include <ranges>

#include "include/algos/partitioner/partition_algo.hpp"

namespace thp {

// partitioner : is a generator of views over a range, like split_view
// algo: is a strategy to generate partition views
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

    decltype(auto) operator * () { return std::ranges::subrange(m_state.start, m_state.end); }

    friend bool operator == (partition_iterator a, partition_iterator b) { return a.m_state == b.m_state; }
  };

  public:
  constexpr explicit partitioner(const PartitionAlgoType& part_algo)
  : algo{std::make_shared<PartitionAlgoType>(part_algo)}
  {}
  inline constexpr decltype(auto) begin() { return partition_iterator(algo, algo->begin()); }
  inline constexpr decltype(auto) end()   { return partition_iterator(algo, algo->end()); }
  inline constexpr std::size_t count() const { return algo->count(); }
};

} // namespace thp
#endif // __PARTITIONER_HPP__
