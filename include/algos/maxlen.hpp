#ifndef MAXLEN_HPP_
#define MAXLEN_HPP_

#include <thread>
#include <memory>
#include <vector>

#include "include/algos/schedule_algos.hpp"
#include "include/statistics.hpp"

namespace thp {
namespace sched_algos {

struct maxlen_algo : schedule_algo {

  void apply(statistics& stats) {
    const auto& inputs = stats.jobq.in.qs;
    auto& output = stats.jobq.out.output;
    if (output.size() < stats.pool.num_workers) {
      auto maxq = *std::ranges::max_element(inputs, {}, [](auto&& q) { return q->len(); });
      auto n = std::min(maxq->len(), stats.pool.num_workers - output.size());
      while(n-- > 0) {
        std::unique_ptr<executable> t;
        if (maxq->pop(t))
          output.emplace_back(std::move(t));
      }
    }
  }

  int apply(statistics& stats, std::thread::id tid)
  {
    return 0;
  }

};

} // namespace sched_algos
} // namespace thp

#endif // MAXLEN_HPP_
