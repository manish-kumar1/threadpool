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

template<typename T>
struct TaskQueueTuple;

template<typename...Ts>
struct TaskQueueTuple<std::tuple<Ts...>> {
  using type = std::tuple<priority_taskq<Ts>...>;
};


// job queue manages several task queues
class job_queue : public std::enable_shared_from_this<job_queue> {
  using TaskQueueTupleType = TaskQueueTuple<AllPriorityTaskTupleType>::type;
  inline consteval decltype(auto) static TaskQueueTupleSize() { return std::tuple_size_v<TaskQueueTupleType>; }

public:
  explicit job_queue()
  : mu{}
  , wmtx{}
  , num_tasks{0}
  , scheduler{}
  , task_qs{create_taskqs_tuple<AllPriorityTaskTupleType>(std::make_index_sequence<TaskQueueTupleSize()>{})}
  , all_qs{}
  , tasks{}
  {
	  create_taskqs_array(task_qs, std::make_index_sequence<std::tuple_size_v<AllPriorityTaskTupleType>>{});
  }

  template <typename C>
  constexpr decltype(auto) insert_task(C&& t) {
    using TaskType = traits::FindTaskType<C>::type;
    auto futs = collect_future(std::forward<C>(t));
    num_tasks += taskq_for<TaskType>().put(std::forward<C>(t));
    return futs;
  }

#if 0
  template<typename Rep, typename Period>
  bool normal_shutdown(const std::chrono::duration<Rep, Period>& rel_time) {
    std::shared_lock l(mu);
    return cond_stop.wait_for(l, rel_time, [this] {
             return (registered_workers == 0) && !pending_tasks();
           });
  }
#endif

  //void copy_stats(jobq_stats& stats);

  //void compute_stats_fn();
  //void schedule_fn(task_scheduler*, std::stop_token st);
  //void worker_fn(std::stop_token st);

  //void request_reschedule();
  //bool update_table();

  void close() {}
  void stop() {}
  void drain();

  bool is_stopped() const;

  void notify_reschedule()      { sched_cond.notify_one(); }

  void schedule_fn(managed_stop_token st) {
    statistics stats{ {std::chrono::system_clock::now(), {all_qs, num_tasks.load(), 2}, {tasks, 0}}, {16, 16} };

    for(;;) {
      thread_local bool ne = false;
      {
        std::shared_lock l(mu);
        ne = sched_cond.wait_for(l, st, Static::scheduler_tick(), [&] { return !empty(); });
        if (st.stop_requested()) break;
        if (st.pause_requested()) {
          l.unlock();
          st.pause();
          l.lock();
        }
      }
      if (ne)
      {
        std::scoped_lock lk(mu, wmtx);
        scheduler.compute_stats(stats);
        scheduler.apply(stats);
      }
      else
        cond_empty.notify_one();
      //if (stats.jobq.out.new_tasks) std::cerr << std::this_thread::get_id() << " notify cond_full: " << num_tasks.load() << std::endl;
      util::notify_cv(cond_full, stats.jobq.out.new_tasks);
    }
  }

  void worker_fn(managed_stop_token st) {
    //std::cerr << "worker_fn: " << st << std::endl;
    for(;;) {
      std::unique_ptr<executable> t{nullptr};
      {
        std::unique_lock l(wmtx);
        cond_full.wait(l, st, [&] {
          return !tasks.empty();
        });
        if (st.stop_requested()) break;
        if (st.pause_requested()) {
          l.unlock();
          st.pause();
          l.lock();
        }
        t = std::move(tasks.front());
        tasks.pop_front();
        --num_tasks;
      }
      t->execute();
      //std::cerr << std::this_thread::get_id() << " got task " << num_tasks.load() << std::endl;
    }
    //std::cerr << std::this_thread::get_id() << " done" << std::endl;
  }

  ~job_queue() = default;

protected:
  friend class scheduler;

  template<typename Tuple, std::size_t... I>
  constexpr void create_taskqs_array(Tuple&& tup, std::index_sequence<I...>)
  {
    all_qs.reserve(TaskQueueTupleSize());
    ((all_qs.emplace_back(std::addressof(std::get<I>(std::forward<Tuple>(tup))))), ...);
  }

  template<typename Tuple, std::size_t...I>
  constexpr TaskQueueTupleType create_taskqs_tuple(std::index_sequence<I...>) {
    return std::make_tuple(priority_taskq<std::tuple_element_t<I, Tuple>>()...);
  }

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

  template <typename TaskType>
  constexpr inline auto& taskq_for(void) {
	// 	return std::get<traits::TaskQType<TaskType>>(task_qs);
    return std::get<priority_taskq<TaskType>>(task_qs);
  }

//  void compute_algo_stats(statistics& stats) {
//    stats.num_tasks = num_tasks;
//  }
//
//  void compute_stats(jobq_stats& stats) {
//    compute_algo_stats(stats.algo);
//  }

  //void check_stop();

  inline bool empty() const { return num_tasks == 0;   }

private:
  mutable std::shared_mutex mu, wmtx;
  std::atomic<int> num_tasks;
  task_scheduler scheduler;
  // task queues for different task types
  TaskQueueTupleType task_qs;
  std::vector<task_queue*> all_qs;
  std::deque<std::unique_ptr<executable>> tasks;
  std::condition_variable_any cond_empty, cond_full, cond_stop, sched_cond;
  int registered_workers;
  bool closed, stopped;
};

} // namespace thp

#endif // JOBQ_HPP_
