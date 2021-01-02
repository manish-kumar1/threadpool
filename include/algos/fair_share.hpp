#ifndef FAIRSHARE_HPP_
#define FAIRSHARE_HPP_

#include <thread>
#include <vector>
#include <random>
#include "include/algos/schedule_algos.hpp"
#include "include/statistics.hpp"

namespace thp {
namespace sched_algos {

class fairshare_algo : public schedule_algo {
private:
  std::random_device rd;

public:
  explicit fairshare_algo() : rd{} {}

  // given length vector, assign workers to proper idx, default is 0
  void apply(const statistics& stats) override {
    const auto& taskq_len = stats.jobq.algo.taskq_len;
    size_t i = rd() % taskq_len.size();
    size_t n = taskq_len.size();

    for(auto& w : **stats.jobq.algo.table) {
      int idx = -1;
      for(i = i == n ? 0 : i; i < n; ++i) {
        if (taskq_len[i] > 0) {
          idx = i++;
          break;
        }
      }
      if (idx == -1) {
        for(i = 0; i < n; ++i) {
          if (taskq_len[i] > 0) {
            idx = i++;
            break;
          }
        }
      }
      if (idx == -1) idx = 0;

      w.second = idx;
    }
  }
      
  int apply(const statistics& stats, std::thread::id tid) override {
    return 0;
  }
};

} // namespace sched_algos
} // namespace thp

#endif // FAIRSHARE_HPP_
