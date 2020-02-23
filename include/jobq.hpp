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

#include "include/traits.hpp"
#include "include/task_queue.hpp"
#include "include/priority_taskq.hpp"
#include "include/task_type.hpp"
#include "include/statistics.hpp"
#include "include/scheduler.hpp"

namespace thp {

class threadpool;

struct jobq_stopped_ex final : std::exception {
  const char *what() const noexcept override { return "jobq_stopped_ex"; }
};

struct jobq_closed_ex final : std::exception {
  const char *what() const noexcept override { return "jobq_closed_ex"; }
};

class job_queue {
public:
  explicit job_queue();

  template<typename InputIter>
  constexpr decltype(auto) insert(InputIter s, InputIter e) {
    using TaskType = std::iterator_traits<InputIter>::value_type::element_type;

    check_stop();
    const auto n = std::distance(s, e);
    taskq_for<TaskType>().insert(std::make_move_iterator(s),
                                 std::make_move_iterator(e));
    num_tasks_ += n;
    cond_full_.notify_all();
  }

  template <typename C, typename = std::enable_if_t<
                            traits::is_container<std::decay_t<C>>::value>>
  constexpr decltype(auto) insert(C&& tasks) {
    using TaskType = typename C::value_type::element_type;

    check_stop();
    const auto n = tasks.size();
    taskq_for<TaskType>().insert(std::make_move_iterator(tasks.begin()),
                                 std::make_move_iterator(tasks.end()));
    num_tasks_ += n;
    cond_full_.notify_all();
  }

  template <typename T>
  constexpr decltype(auto) insert_task(T &&t) {
    using TaskType = typename T::element_type;
    static_assert(traits::is_smart_ptr<T>::value && std::is_base_of_v<task, TaskType>);
    taskq_for<TaskType>().put(std::forward<T>(t));
    return true;
  }

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

  const bool &is_stopped() const;

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

    std::unique_lock l(mu_);
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
