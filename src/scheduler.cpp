#include <memory>
#include <thread>
#include <shared_mutex>
#include <mutex>

#include "include/worker_pool.hpp"
#include "include/scheduler.hpp"
#include "include/algos/scheduling/schedule_algos.hpp"
#include "include/algos/scheduling/maxlen.hpp"
#include "include/algos/scheduling/first_available.hpp"
#include "include/algos/scheduling/fair_share.hpp"

namespace thp {
task_scheduler::task_scheduler()
  : all_algos{}
  , algo{nullptr}
  {
    all_algos[sched_algos::names::eFirstAvailable].reset(new sched_algos::first_avail_algo());
    all_algos[sched_algos::names::eMaxLen].reset(new sched_algos::maxlen_algo());
    all_algos[sched_algos::names::eFairShare].reset(new sched_algos::fairshare_algo());
    algo = all_algos[sched_algos::names::eFairShare];
  }

void task_scheduler::compute_stats(statistics& stats) {
}

void task_scheduler::apply(statistics& stats) {
  algo->apply(stats);
}

#if 0
void task_scheduler::reschedule(std::thread_id id) {
  {
    std::unique_lock l(mu);
    //reschedule = true;
  }
  sched_cond.notify_one();
}




void task_scheduler::reschedule_worker(const statistics& stats, std::thread::id tid) {
  //return algo_->apply(stats, tid);
}

bool task_scheduler::reschedule(const statistics& stats) {
  auto ret = algo_->ok(stats);
  if (!ret)
    algo_->apply(stats);

  return !ret;

}
#endif
} // namespace thp
