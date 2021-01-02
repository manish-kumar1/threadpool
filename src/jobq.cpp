#include <algorithm>
#include <condition_variable>
#include <exception>
#include <functional>
#include <future>
#include <iterator>
#include <memory>
#include <tuple>
#include <typeindex>
#include <stop_token>

#include "include/threadpool.hpp"
#include "include/traits.hpp"
#include "include/task_queue.hpp"
#include "include/priority_taskq.hpp"
#include "include/task_type.hpp"
#include "include/worker_pool.hpp"

namespace thp {

job_queue::job_queue()
    : mu_{}
    , num_tasks_{0}
    , idx_{}
    , tasks_qs_{}
    , worker_taskq_map_{}
    , table_{&worker_taskq_map_[0]}
    , table_old_{&worker_taskq_map_[1]}
    , cond_empty_{}, cond_full_{}, cond_stop_{}
    , registered_workers_{0}
    , closed_{false}
    , stopped_{false}
    , reschedule_{false}
{
  idx_.reserve(Static::queue_table_capacity());
  tasks_qs_.emplace_back(new task_queue());
  idx_.emplace(std::make_pair(std::type_index(typeid(void)), 0));
}

void job_queue::compute_algo_stats(sched_algo_stats& stats) {
  stats.taskq_len.clear();
  stats.taskq_len.resize(tasks_qs_.size());
  stats.num_tasks = num_tasks_;
  std::transform(tasks_qs_.cbegin(), tasks_qs_.cend(),
                 std::inserter(stats.taskq_len, stats.taskq_len.begin()),
                 [](auto&& q) { return q->len(); });
  stats.table = &table_old_;
} 

void job_queue::compute_stats(jobq_stats& stats) {
    // compute stats
  stats.ts = std::chrono::system_clock::now();
  stats.worker_2_q = *table_;
  compute_algo_stats(stats.algo);
}

void job_queue::copy_stats(jobq_stats& stats) {
  std::lock_guard l(mu_);
  compute_stats(stats);
}

void job_queue::schedule_fn(task_scheduler* scheduler, std::stop_token st)
{
  statistics stats;

  for(;;) {
    std::unique_lock l(mu_);

    sched_cond_.wait_for(l, Static::scheduler_tick(), [&] { return reschedule_ ;});
    if (st.stop_requested()) break;
    if (reschedule_ || num_tasks_ > 0) {
      reschedule_ = false;
      compute_algo_stats(stats.jobq.algo);
      l.unlock();

      auto update = scheduler->reschedule(stats);
      if (update) {
        l.lock();
        std::swap(table_old_, table_);
        l.unlock();
        cond_full_.notify_all();
      }
    }
  }
}

void job_queue::worker_fn(std::stop_token st) {
  const auto me = std::this_thread::get_id();
  register_worker(me);

  for(;;) {
    std::unique_ptr<executable> t{};
    {
      std::shared_lock l(mu_);
      cond_full_.wait(l, st, [&] {
        bool ret = true;
        if (!pending_tasks()) ret = false;
        else if (!tasks_qs_[table_->at(me)]->pop(t)) {
            ret = false;
            reschedule_ = true;
        }
        return ret;
      });

      if (st.stop_requested())
        break;
    }

    t->execute();

    if (0 != num_tasks_)
      --num_tasks_;

    if (0 == num_tasks_)
      cond_empty_.notify_all();
  }

  deregister_worker(me);
}

bool job_queue::update_table() {
  bool ret = false;
  {
    std::unique_lock l(mu_, std::defer_lock);
    if((ret = l.try_lock()))
      std::swap(table_, table_old_);
  }
  return ret;
}

void job_queue::close() {
  std::unique_lock l(mu_);
  closed_ = true;
}

void job_queue::stop() {
  {
    std::unique_lock l(mu_);
    stopped_ = true;
  }

  do {
    cond_full_.notify_all();
    cond_empty_.notify_all();
  }
  while(!normal_shutdown(Static::shutdown_grace_period()));
}

void job_queue::drain() {
  cond_full_.notify_all();
  std::shared_lock l(mu_);
  cond_empty_.wait(l, [this] { return !pending_tasks(); });
}

void job_queue::register_worker(const std::thread::id& tid) {
  std::lock_guard l(mu_);
  worker_taskq_map_[0][tid] = 0;
  worker_taskq_map_[1][tid] = 0;
  ++registered_workers_;
}

void job_queue::deregister_worker(const std::thread::id& tid) {
  std::lock_guard l(mu_);
  worker_taskq_map_[0][tid] = 0;
  worker_taskq_map_[1][tid] = 0;
  --registered_workers_;
}

void job_queue::check_stop() {
  std::lock_guard l(mu_);
  if (closed_)
    throw jobq_closed_ex();
  else if (stopped_)
    throw jobq_stopped_ex();
}

bool job_queue::is_stopped() const {
  std::shared_lock l(mu_);
  return stopped_;
}

} // namespace thp
