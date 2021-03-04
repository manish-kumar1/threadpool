#ifndef JOBQ_HPP_
#define JOBQ_HPP_

#include <algorithm>
#include <condition_variable>
#include <future>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <chrono>
#include <array>

#include "include/scheduler.hpp"
#include "include/traits.hpp"
#include "include/task_queue.hpp"
#include "include/task_type.hpp"
#include "include/statistics.hpp"
#include "include/util.hpp"
#include "include/all_priority_types.hpp"
#include "include/managed_stop_token.hpp"

namespace thp {

class threadpool;

struct jobq_stopped_ex final : std::exception {
  const char *what() const noexcept override { return "jobq_stopped_ex"; }
};

struct jobq_closed_ex final : std::exception {
  const char *what() const noexcept override { return "jobq_closed_ex"; }
};

// job queue manages several task queues
template <typename TaskQueueTupleType>
class job_queue {
  //using TaskQueueTupleType = TaskQueueTuple<Tup>::type;
  static const auto NumQs = std::tuple_size_v<TaskQueueTupleType>;

public:
  explicit job_queue()
  : mu{}
  , wmtx{}
  , num_tasks{0}
  , busy_workers{0}
  , scheduler{}
  , task_qs{create_taskqs_tuple(std::make_index_sequence<NumQs>{})}
  , all_qs{}
  , tasks{}
  , cur_output{&tasks[0]}
  , old_output{&tasks[1]}
  {
    create_taskqs_array(task_qs, std::make_index_sequence<NumQs>{});
  }

  template <typename C>
  constexpr decltype(auto) schedule_task(C&& t) {
    auto futs = collect_future(std::forward<C>(t));
    insert_task(std::forward<C>(t));
    return futs;
  }

  template <typename C>
  constexpr void insert_task(C&& t) {
    using TaskType = traits::FindTaskType<C>::type;
    num_tasks += taskq_for<TaskType>().put(std::forward<C>(t));
    sched_cond.notify_one();
  }

  void close() {}
  void stop() {}

  void drain() {
    cond_full.notify_all();
    std::unique_lock l(wmtx);
    cond_empty.wait(l, [this] { return (busy_workers == 0) && empty() && cur_output->empty() && old_output->empty(); });
  }

  void notify_reschedule()      { sched_cond.notify_one(); }

  void schedule_fn(managed_stop_token st) {
    statistics stats{std::chrono::system_clock::now(), {{all_qs, num_tasks.load(), -1}, {cur_output, 0}}, {16, 16} };
    unsigned last_num_tasks = 0u;
    auto cur_num_tasks = num_tasks.load();

    for(;;) {
      thread_local bool ne = false;
      stats.jobq.out.reset();
      stats.jobq.out.cur_output = old_output;
      {
        std::shared_lock l(mu);
        //auto x = std::chrono::high_resolution_clock::now();
        ne = sched_cond.wait(l, st, [&] {
              //auto cur_num_tasks = num_tasks.load();
              const auto need_schedule = (num_tasks > 0) || (!old_output->empty());
              //std::cerr << std::this_thread::get_id() << " schedule_fn1: " << num_tasks.load() << ", " << cur_num_tasks << ", " << need_schedule << ", " << old_output->size() << std::endl;
              if (!need_schedule) cond_empty.notify_all();
              return need_schedule;
            });
        //auto y = std::chrono::high_resolution_clock::now();
        //std::cerr << "scheduler_tick: " <<  std::chrono::duration<double, std::micro>(y-x).count() << std::endl;
        if (st.stop_requested()) break;
        if (st.pause_requested()) {
          l.unlock();
          st.pause();
          l.lock();
        }
      }
      if (ne)
      {
        //  compute_stats(stats);
        if (num_tasks > 0) {
          scheduler.apply(stats);
          num_tasks -= stats.jobq.out.new_tasks;
        }
        {
          std::unique_lock l(wmtx);
          if (cur_output->empty()) {
            std::swap(cur_output, old_output);
            stats.jobq.out.new_tasks = cur_output->size();
            //num_tasks -= cur_output->size();
          }
        }
      }

      //std::cerr << std::this_thread::get_id() << " schedule_fn2: " << num_tasks.load() << ", " << stats.jobq.out.new_tasks << ", " << ne << ", " << old_output->size() << ", " << stats.jobq.out.new_tasks << std::endl;
      //if (stats.jobq.out.new_tasks) std::cerr << std::this_thread::get_id() << " notify cond_full: " << num_tasks.load() << ", " << stats.jobq.out.new_tasks << std::endl;
      util::notify_cv(cond_full, stats.jobq.out.new_tasks);
    }
  }

  void worker_fn(managed_stop_token st) {
    //std::cerr << "worker_fn: " << st << std::endl;
    for(;;) {
      std::shared_ptr<executable> t{nullptr};
      {
        std::unique_lock l(wmtx);
        cond_full.wait(l, st, [&] {
          if (cur_output->empty()) {
            sched_cond.notify_one();
            return false;
          }
          else 
            return true;
        });
        if (st.stop_requested()) break;
        if (st.pause_requested()) {
          l.unlock();
          st.pause();
          l.lock();
        }
        t = std::move(cur_output->front());
        cur_output->pop_front();
      }
      //std::cerr << std::this_thread::get_id() << " working" << std::endl;

      ++busy_workers;
      t->execute();
      --busy_workers;
    }
    //std::cerr << std::this_thread::get_id() << " done" << std::endl;
  }

  //~job_queue() = default;

  template <typename C>
  constexpr decltype(auto) collect_future(C&& t)
  {
    using T = std::remove_cvref_t<decltype(t)>;

    if constexpr (traits::is_unique_ptr<T>::value)                 { return t->future(); }
    else if constexpr (traits::is_shared_ptr<T>::value)            { return t->future(); }
    else if constexpr (traits::is_reference_wrapper<T>::value)     { return t.get().future(); }
    else if constexpr (std::is_base_of_v<executable, T>)           { return t.future(); }
    else if constexpr (traits::is_container<T>::value) {
      using TaskType = traits::FindTaskType<std::remove_cvref_t<typename T::value_type>>::type;
      using ReturnType = typename TaskType::ReturnType;
      std::vector<std::future<ReturnType>> futs;
      futs.reserve(t.size());
      std::ranges::transform(t, std::back_inserter(futs),
                             [&](auto&& x) { return collect_future(x); });
      return futs;
    }
    else {
      static_assert("type is not supported");
    }
  }

  std::size_t size() const {
    std::scoped_lock l(mu, wmtx);
    return num_tasks + cur_output->size() + old_output->size();
  }

protected:

  friend class scheduler;

  template<typename Tuple, std::size_t... I>
  constexpr void create_taskqs_array(Tuple&& tup, std::index_sequence<I...>)
  {
    all_qs.reserve(NumQs);
    ((all_qs.emplace_back(std::addressof(std::get<I>(std::forward<Tuple>(tup))))), ...);
  }

  template<std::size_t...I>
  constexpr TaskQueueTupleType create_taskqs_tuple(std::index_sequence<I...>) {
    return std::make_tuple(std::tuple_element_t<I, TaskQueueTupleType>()...);
  }

  template <typename TaskType>
  constexpr inline auto& taskq_for(void) {
	  using T = priority_taskq<typename TaskType::PriorityType>;
    return std::get<T>(task_qs);
  }

//  void compute_algo_stats(statistics& stats) {
//    stats.num_tasks = num_tasks.load();
//  }
//
//  void compute_stats(jobq_stats& stats) {
//    compute_algo_stats(stats.algo);
//  }

  inline bool empty() const { return num_tasks == 0;   }

private:
  mutable std::shared_mutex mu, wmtx;
  std::atomic<int> num_tasks, busy_workers;
  task_scheduler scheduler;
  // task queues for different task types
  TaskQueueTupleType task_qs;
  std::vector<task_queue*> all_qs;
  std::deque<std::shared_ptr<executable>> tasks[2], *cur_output, *old_output;
  std::condition_variable_any cond_empty, cond_full, cond_stop, sched_cond;
  int registered_workers;
  bool closed, stopped;
};

} // namespace thp

#endif // JOBQ_HPP_
