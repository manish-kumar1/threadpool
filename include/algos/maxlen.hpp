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
  explicit maxlen_algo() : last_idx_{0}, cur_idx_{0} {}

  int best_queue_index(const std::vector<unsigned>& len) {
    auto it = std::max_element(len.begin(), len.end());

    if (it == len.end())
      it = len.begin();

    return std::distance(len.begin(), it);
  }

  bool ok(const statistics& stats) override {
    auto tmp = cur_idx_;
    bool ret = (last_idx_ == (cur_idx_ = best_queue_index(stats.jobq.algo.taskq_len)));
    last_idx_ = tmp;
    return ret;
  }

  void apply(const statistics& stats) {
    for(auto& w : **stats.jobq.algo.table)
      w.second = cur_idx_;
  }

  int apply(const statistics& stats, std::thread::id tid)
  {
    (**stats.jobq.algo.table)[tid] = cur_idx_;
    return cur_idx_;
  }

private:
  int last_idx_, cur_idx_;
};

} // namespace sched_algos
} // namespace thp

#endif // MAXLEN_HPP_
