#ifndef SCHEDULER_HPP_
#define SCHEDULER_HPP_

#include <memory>
#include <unordered_map>

#include "include/algos/schedule_algos.hpp"
#include "include/statistics.hpp"

namespace thp {

class task_scheduler {
public:
  explicit task_scheduler();
  bool reschedule(const statistics& stats);
  int reschedule_worker(const statistics&, std::thread::id);

  ~task_scheduler() {};

private:
  std::unordered_map<sched_algos::names, std::shared_ptr<sched_algos::schedule_algo>> all_algos_;
  std::shared_ptr<sched_algos::schedule_algo> algo_;
};

} // namespace thp

#endif // SCHEDULER_HPP_
