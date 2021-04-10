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

struct jobq_stopped_ex final : std::exception {
  const char *what() const noexcept override { return "jobq_stopped_ex"; }
};

struct jobq_closed_ex final : std::exception {
  const char *what() const noexcept override { return "jobq_closed_ex"; }
};

// job queue manages several task queues
template <typename TaskQueueTupleType>
class job_queue {
  static const auto NumQs = std::tuple_size_v<TaskQueueTupleType>;

public:
  constexpr explicit job_queue()
  : mu{}
  , wmtx{}
  , num_tasks{0}
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
    auto n = insert_task(std::forward<C>(t));
    {
      std::lock_guard l(mu);
      num_tasks += n;
    }
    sched_cond.notify_one();
    return futs;
  }

  void close() {}
  void stop() {}

  void notify_reschedule()      { sched_cond.notify_one(); }
#if 0
  // scheduler
  // worker : tsq, one-shot, time-series, seq, par, vectorized
  struct worker_config {
    constexpr decltype(auto) initializer() { return std::optional<function; }
    constexpr decltype(auto) work_loop()   { return ; }
    constexpr decltype(auto) finalizer()   { return ; }

    std::variant<SchedulerConfig, WorkerConfig, BookKeeper> data;
  };

  void worker_fn(std::shared_ptr<worker_config> config) {
    for(;;) {
      try {
        auto init_lambda = config->initializer();
        auto work_loop   = config->work_loop();
        auto finalizer   = config->finalizer();

        if (init_lambda) std::invoke(init_lambda.value());
        if (work_loop) std::invoke(work_loop.value(), config.stop_token());
      }
      catch(worker_config_change_except &ex) {}
      catch(worker_stop_except& ex {
        if (finalizer) std::invoke(finalizer.value());
        break;
      }
      catch(...) {
        throw;
      }
    }
  }
#endif
  void schedule_fn(managed_stop_token st) {
    statistics stats{std::chrono::system_clock::now(), {{all_qs, num_tasks, -1}, {cur_output, 0}}, {16, 16} };
    unsigned pending_tasks = 0u;
    for(;;) {
      thread_local bool ne = false;
      stats.jobq.out.reset();
      stats.jobq.out.cur_output = old_output;
      {
        std::unique_lock l(mu);
        ne = sched_cond.wait(l, st, [&] {
              if (num_tasks > 0) {
                scheduler.apply(stats);
                num_tasks -= stats.jobq.out.new_tasks;
              }
              return !old_output->empty();
             });
        if (st.stop_requested()) [[unlikely]] break;
#if 0
        else if (st.pause_requested()) [[unlikely]] {
          l.unlock();
          st.pause();
          l.lock();
        }
#endif
      }
      if (ne) {
        {
          std::unique_lock l(wmtx);
          sched_cond.wait(l, st, [&] { return cur_output->empty(); });
          std::swap(cur_output, old_output);
          //idx = 0;
          //old_output->clear();
          stats.jobq.out.new_tasks = cur_output->size();
        }
        util::notify_cv(cond_full, stats.jobq.out.new_tasks);
      }

    //std::cerr << std::this_thread::get_id() << " schedule_fn: " << pending_tasks << ", " << stats.jobq.out.new_tasks << ", " << ne << ", " << old_output->size() << std::endl;
    }
  }

  void worker_fn2(managed_stop_token st) {  // todo
    thread_local const unsigned my_idx = std::atomic_fetch_add_explicit(&idx, 1, std::memory_order::acq_rel);
    for(;;) {
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
        if (st.stop_requested()) [[unlikely]] break;
      }
      const auto n = cur_output->size();
      for(thread_local std::size_t i = my_idx; i < n; i += 16) {
        {
          //std::cerr << std::this_thread::get_id() << " work[" << i << "] " << n << "\n";
        }
        cur_output->at(i)->execute();
      }
      std::atomic_fetch_sub_explicit(&idx, 1, std::memory_order::acq_rel);
    }
  }

  void worker_fn(managed_stop_token st) {
    for(;;) {
      thread_local std::unique_ptr<executable> t{nullptr};
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
        if (st.stop_requested()) [[unlikely]] break;
#if 0
        else if (st.pause_requested()) [[unlikely]] {
          l.unlock();
          st.pause();
          l.lock();
          continue;
        }
#endif
        t = std::move(cur_output->front());
        cur_output->pop_front();
      }

      t->execute();
    }
  }

  template <typename C>
  constexpr decltype(auto) collect_future(C&& t)
  {
    using T = std::remove_cvref_t<decltype(t)>;

    if      constexpr (traits::is_unique_ptr<T>::value)            { return t->future(); }
    else if constexpr (traits::is_shared_ptr<T>::value)            { return t->future(); }
    else if constexpr (traits::is_reference_wrapper<T>::value)     { return t.get().future(); }
    else if constexpr (std::is_base_of_v<executable, T>)           { return t.future(); }
    else if constexpr (traits::is_vector<T>::value || traits::is_linked_list<T>::value) {
      using TaskType = traits::FindTaskType<std::remove_cvref_t<typename T::value_type>>::type;
      using ReturnType = typename TaskType::ReturnType;
      std::deque<std::future<ReturnType>> futs;
      std::ranges::transform(t, std::back_inserter(futs),
                             [&](auto&& x) { return collect_future(x); });
      return futs;
    }
    else {
      static_assert("type is not supported");
    }
  }

  std::size_t size() const {
    return num_tasks;
  }

protected:

  //friend class scheduler;

  template <typename C>
  constexpr std::size_t insert_task(C&& t) {
    using TaskType = traits::FindTaskType<C>::type;
    return taskq_for<TaskType>().put(std::forward<C>(t));
  }

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

private:
  mutable std::mutex mu;
  mutable std::mutex wmtx;
  unsigned num_tasks;
  std::atomic<int> idx;
  task_scheduler scheduler;
  // task queues for different task types
  TaskQueueTupleType task_qs;
  std::vector<task_queue*> all_qs;
  std::deque<std::unique_ptr<executable>> tasks[2], *cur_output, *old_output;
  std::condition_variable_any cond_empty, cond_full, cond_stop, sched_cond;
  bool closed, stopped;
};

} // namespace thp

#endif // JOBQ_HPP_
