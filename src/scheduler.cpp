#include <memory>
#include <thread>
#include <shared_mutex>
#include <mutex>

#include "include/worker_pool.hpp"
#include "include/scheduler.hpp"
#include "include/algos/schedule_algos.hpp"
#include "include/algos/maxlen.hpp"
#include "include/algos/first_available.hpp"
#include "include/algos/fair_share.hpp"

namespace thp {
task_scheduler::task_scheduler()
  : all_algos_{}
  , algo_{nullptr}
  {
    all_algos_[sched_algos::names::eFirstAvailable].reset(new sched_algos::first_avail_algo());
    all_algos_[sched_algos::names::eMaxLen].reset(new sched_algos::maxlen_algo());
    all_algos_[sched_algos::names::eFairShare].reset(new sched_algos::fairshare_algo());
    algo_ = all_algos_[sched_algos::names::eFairShare];
  }

int task_scheduler::reschedule_worker(const statistics& stats, std::thread::id tid) {
  return algo_->apply(stats, tid);
}

bool task_scheduler::reschedule(const statistics& stats) {
  auto ret = algo_->ok(stats);
  if (!ret)
    algo_->apply(stats);

  return !ret;
}

} // namespace thp
