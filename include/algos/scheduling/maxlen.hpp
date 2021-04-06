#ifndef MAXLEN_HPP_
#define MAXLEN_HPP_

#include <thread>
#include <memory>
#include <vector>

#include "include/algos/scheduling/schedule_algos.hpp"
#include "include/statistics.hpp"

namespace thp {
namespace sched_algos {

struct maxlen_algo : schedule_algo {

  void apply(statistics& stats) {
    const auto& inputs = stats.jobq.in.qs;
    auto& output = *stats.jobq.out.cur_output;
    auto expected_items = stats.jobq.in.load_factor * stats.pool.num_workers;
    if (output.size() < expected_items) {
      auto maxq = *std::ranges::max_element(inputs, {}, &task_queue::len);
      stats.jobq.out.new_tasks = maxq->pop_n(output, expected_items);
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
