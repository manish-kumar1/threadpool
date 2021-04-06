#ifndef SCHEDULER_HPP_
#define SCHEDULER_HPP_

#include <memory>
#include <unordered_map>

#include "include/algos/scheduling/schedule_algos.hpp"
#include "include/statistics.hpp"
#include "include/managed_stop_token.hpp"

namespace thp {

class task_scheduler {
public:
  explicit task_scheduler();
  void update_algorithm(sched_algos::names new_algo);
  void compute_stats(statistics& stats);
  void reschedule_worker(const statistics&, std::thread::id);
  void apply(statistics& stats);

  virtual ~task_scheduler() = default;

protected:
  void register_worker(std::thread::id);
  void deregister_worker(std::thread::id);

private:
  std::unordered_map<sched_algos::names, std::shared_ptr<sched_algos::schedule_algo>> all_algos;
  std::shared_ptr<sched_algos::schedule_algo> algo;
};

} // namespace thp

#endif // SCHEDULER_HPP_
