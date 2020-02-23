#ifndef FIRST_AVAILABLE_HPP_
#define FIRST_AVAILABLE_HPP_

#include <thread>

#include "include/algos/schedule_algos.hpp"
#include "include/statistics.hpp"

namespace thp {
namespace sched_algos {

struct first_avail_algo : schedule_algo {
  explicit first_avail_algo()
    : cur_idx_{0}
    , last_idx_{0}
  {}

  size_t best_idx(const statistics& stats) {
    auto& v = stats.jobq.algo.taskq_len;
    auto it = std::find_if(v.begin(), v.end(), [](auto&& x) { return x > 0; });
    if (it == v.end())
      it = v.begin();

    return std::distance(v.begin(), it);
  }

  bool ok(const statistics& stats) {
    auto tmp = cur_idx_;
    bool ret = (last_idx_ == (cur_idx_ = best_idx(stats)));
    last_idx_ = tmp;
    return ret;
  }

  void apply(const statistics& stats) override {
    for(auto& p : **stats.jobq.algo.table) {
      p.second = cur_idx_;
    }
  }
 
  int apply(const statistics& stats, std::thread::id tid) override {
    (**stats.jobq.algo.table)[tid] = cur_idx_;
    return cur_idx_;
  }
protected:
  size_t cur_idx_, last_idx_;
};

} // namespace sched_algos
} // namespace thp

#endif // FIRST_AVAILABLE_HPP_
