#ifndef JOBQ_HPP_
#define JOBQ_HPP_

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
#include <tuple>

#include "include/traits.hpp"
#include "include/task_queue.hpp"
#include "include/priority_taskq.hpp"
#include "include/task_type.hpp"
#include "include/statistics.hpp"
#include "include/scheduler.hpp"
#include "include/util.hpp"

namespace thp {

class threadpool;

template<typename> struct is_tuple : std::false_type {};
template<typename... T> struct is_tuple<std::tuple<T...>> : std::true_type {};


template<typename T1, typename T2, typename = std::enable_if_t<
  is_tuple<T1>::value && is_tuple<T2>::value>>
static decltype(auto) operator + (T1&& t1, T2&& t2) { return std::tuple_cat(std::forward<T1>(t1), std::forward<T2>(t2)); }

struct jobq_stopped_ex final : std::exception {
  const char *what() const noexcept override { return "jobq_stopped_ex"; }
};

struct jobq_closed_ex final : std::exception {
  const char *what() const noexcept override { return "jobq_closed_ex"; }
};

class job_queue {
public:
  explicit job_queue();

  template <typename C>
  constexpr decltype(auto) insert_task(C&& t) {
    using T = std::remove_cvref_t<C>;
    if constexpr (traits::is_smart_ptr<T>::value) {
      using TaskType = std::decay_t<typename T::element_type>;
      auto fut = t->future();
      taskq_for<TaskType>().put(std::forward<T>(t));
      num_tasks_ += 1;
      cond_full_.notify_one();
      return fut;
    }
    else if constexpr (std::is_base_of_v<executable, T>) {
      auto fut = t.future();
      taskq_for<std::decay_t<T>>().put(std::forward<T>(t));
      num_tasks_ += 1;
      cond_full_.notify_one();
      return fut;
    }
    else if (traits::is_container<T>::value) {
      using P = typename T::value_type;

      if constexpr (traits::is_smart_ptr<P>::value) {
        using TaskType = typename P::element_type;
        using ReturnType = typename TaskType::ReturnType;
  
        static_assert(traits::is_smart_ptr<P>::value && std::is_base_of_v<executable, TaskType>);

        auto N = t.size();
        std::vector<std::future<ReturnType>> futs;
        futs.reserve(N);
        std::transform(t.begin(), t.end(), std::back_inserter(futs),
                      [](auto&& w) { return w->future(); });
    
        taskq_for<TaskType>().insert(std::make_move_iterator(t.begin()),
                                     std::make_move_iterator(t.end()));
        num_tasks_ += N;
        util::notify_cv(cond_full_, N);
        return futs;
      }
      else if constexpr (std::is_base_of_v<executable, P>) {
        using TaskType = std::remove_cvref_t<P>;
        using ReturnType = typename TaskType::ReturnType;

        auto N = t.size();
        std::vector<std::future<ReturnType>> futs;
        futs.reserve(N);
        std::transform(t.begin(), t.end(), std::back_inserter(futs),
                      [](auto&& w) { return w.future(); });

        std::vector<std::unique_ptr<TaskType>> tasks;
        std::transform(t.begin(), t.end(), std::back_inserter(tasks),
                        [](auto&& w) { return std::make_unique<TaskType>(std::move(w)); });
        taskq_for<TaskType>().insert(std::make_move_iterator(tasks.begin()),
                                     std::make_move_iterator(tasks.end()));
        num_tasks_ += N;
        util::notify_cv(cond_full_, N);
        return futs;
      }
      else
        static_assert("type not supported");
    }
  else
    static_assert("type not supported");
  }

  template <typename... Args>
  constexpr decltype(auto) insert(Args&&... args) {
    return (std::make_tuple() + ... + std::make_tuple(insert_task(std::forward<Args>(args))));
  }

#if 0
  template <typename... Args>
  constexpr decltype(auto) insert(std::tuple<Args...>&& tasks) {
    check_stop();
    auto tup = std::apply(
        [this](auto &&... x) {
          return std::make_tuple(insert_task(std::move(x))...);
        },
        std::move(tasks));

    num_tasks_ += std::tuple_size_v<decltype(tup)>;
    cond_full_.notify_all();
  }
#endif

  template<typename Rep, typename Period>
  bool normal_shutdown(const std::chrono::duration<Rep, Period>& rel_time) {
    std::shared_lock l(mu_);
    return cond_stop_.wait_for(l, rel_time, [this] {
             return (registered_workers_ == 0) && !pending_tasks();
           });
  }

  void copy_stats(jobq_stats& stats);

  void compute_stats_fn();
  void schedule_fn(task_scheduler*, std::stop_token st);
  void worker_fn(std::stop_token st);

  void request_reschedule();
  bool update_table();

  void close();
  void stop();
  void drain();

  bool is_stopped() const;

  void register_worker();
  void deregister_worker();

  ~job_queue() = default;

protected:
  void compute_algo_stats(sched_algo_stats& );
  void compute_stats(jobq_stats& stats);

  void check_stop();

  inline bool pending_tasks(void) { return num_tasks_ > 0; }

  template <typename TaskType>
  constexpr task_queue &taskq_for(void) {
    using PriorityType = std::decay_t<typename TaskType::PriorityType>;

    //std::unique_lock l(mu_);
    auto x = std::type_index(typeid(PriorityType));
    auto it = idx_.find(x);
    if (it == idx_.end()) {
      tasks_qs_.emplace_back(std::make_unique<priority_taskq>());
      std::tie(it, std::ignore) = idx_.emplace(x, tasks_qs_.size() - 1);
    }
    return *tasks_qs_[it->second];
  }

private:
  mutable std::shared_mutex mu_;
  std::atomic<int> num_tasks_;
  // task queues for different task types
  std::unordered_map<std::type_index, int> idx_;
  std::deque<std::unique_ptr<task_queue>> tasks_qs_;
  // worker's task queue map
  std::array<std::unordered_map<std::thread::id, int>, 2> worker_taskq_map_;
  std::unordered_map<std::thread::id, int>* table_;
  std::unordered_map<std::thread::id, int>* table_old_;
  
  std::condition_variable_any cond_empty_, cond_full_, cond_stop_, sched_cond_;
  int registered_workers_;
  bool closed_, stopped_;
  bool reschedule_;
};

} // namespace thp

#endif // JOBQ_HPP_
