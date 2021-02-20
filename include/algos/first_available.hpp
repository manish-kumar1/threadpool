#ifndef FIRST_AVAILABLE_HPP_
#define FIRST_AVAILABLE_HPP_

#include <thread>

#include "include/algos/schedule_algos.hpp"
#include "include/statistics.hpp"
#include "include/task_queue.hpp"

namespace thp {
namespace sched_algos {

struct first_avail_algo : schedule_algo {
  void apply(statistics& stats) override {
    const auto& inputs = stats.jobq.in.qs;
    auto& output = stats.jobq.out.cur_output;
    auto expected_items = stats.jobq.in.load_factor * stats.pool.num_workers;
    if (output.size() < expected_items) {
      auto fq = std::ranges::find_if(inputs, [](auto&& x) { return x > 0; }, &task_queue::len);
      if (fq != inputs.end())
        stats.jobq.out.new_tasks = (*fq)->pop_n(output, expected_items);
    }
  }

  int apply(statistics& stats, std::thread::id tid) override {
    return 0;
  }
protected:
  size_t cur_idx_, last_idx_;
};

} // namespace sched_algos
} // namespace thp

#endif // FIRST_AVAILABLE_HPP_
