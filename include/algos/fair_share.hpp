#ifndef FAIRSHARE_HPP_
#define FAIRSHARE_HPP_

#include <thread>
#include <vector>
#include <random>
#include <ranges>

#include "include/algos/schedule_algos.hpp"
#include "include/statistics.hpp"

namespace thp {
namespace sched_algos {

class fairshare_algo : public schedule_algo {
private:
  std::random_device rd;

public:
  explicit fairshare_algo() : rd{} {}

  void apply(statistics& stats) override {
    const auto& inputs = stats.jobq.in.qs;
    auto& output = stats.jobq.out.cur_output; // TODO(find out why auto& works and auto doesn't)
    auto n = output.size();
    auto expected_items = stats.jobq.in.load_factor * stats.pool.num_workers;

    if (stats.jobq.in.load_factor < 0) {
      // put all tasks in queue
      std::ranges::for_each(inputs, [&](auto&& q) {
        q->pop_n(output, q->len());
      });
    }
    else if (output.size() < expected_items) {
      std::ranges::for_each(inputs, [&](auto&& q) {
                   std::shared_ptr<executable> t;
                   if (q->pop(t))
                     output.emplace_back(std::move(t));
               });
    }
    stats.jobq.out.new_tasks = output.size() - n;
  }
      
  int apply(statistics& stats, std::thread::id tid) override {
    return 0;
  }
};

} // namespace sched_algos
} // namespace thp

#endif // FAIRSHARE_HPP_
