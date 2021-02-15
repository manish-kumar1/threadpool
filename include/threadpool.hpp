#ifndef THREADPOOL_HPP_
#define THREADPOOL_HPP_

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <numeric>
#include <tuple>
#include <type_traits>
#include <vector>
#include <stop_token>

#include "include/util.hpp"
#include "include/jobq.hpp"
#include "include/scheduler.hpp"
#include "include/managed_thread.hpp"
#include "include/signal_handler.hpp"
#include "include/task_type.hpp"
#include "include/worker_pool.hpp"
#include "include/task_factory.hpp"

namespace thp {

class threadpool {
public:
  explicit threadpool(unsigned max_threads = std::thread::hardware_concurrency());

  // waits till condition of no tasks is satisfied
  void drain();

  // pause
  void pause();
  void resume();

  // graceful shutdown
  void shutdown();

  // could be heterogeneous task types
  template <typename... Args>
  constexpr decltype(auto) schedule(Args&&... args) {
    auto futs = std::make_tuple(jobq_.insert_task(args)...);
    jobq_.notify_reschedule();
    return futs;
  }

  template <typename Clock> 
  constexpr decltype(auto) run_for(typename Clock::duration dur) {}

  template <typename Task, typename...Callables>
  constexpr decltype(auto) chain(Task&& t, Callables&&... fn) {}

  template <typename Fn, typename... Args>
  requires std::invocable<Fn,Args...>
  constexpr decltype(auto) enqueue(Fn&& fn, Args&&... args) {
    return schedule(make_task(std::forward<Fn>(fn), std::forward<Args>(args)...));
  }

  // parallel algorithm, for benchmarks see examples/reduce.cpp
  template <
    std::input_iterator I, std::sentinel_for<I> S,
    typename T,
    typename BinaryOp,
    typename Partitioner
  >
  requires std::movable<T>
  decltype(auto) reduce(I s, S e, T init, BinaryOp rdc_fn, Partitioner part) {
    return transform_reduce(s, e, std::move(init), rdc_fn, std::identity(), part);
  }

  template <
    std::input_iterator I, std::sentinel_for<I> S,
    typename T,
    typename BinaryOp,
    typename UnaryOp,
    typename Partitioner
  >
  requires std::movable<T>
  decltype(auto) transform_reduce(I s, S e, T init,
                                  BinaryOp rdc_fn, UnaryOp tr_fn,
                                  Partitioner part) {
    auto transform_reduce_fn = [=](auto &&s1, auto &&e1) {
      return std::transform_reduce(s1, e1, init, rdc_fn, tr_fn);
    };

    std::vector<std::shared_ptr<simple_task<T>>> tasks;
    tasks.reserve(part.count());

    for (auto it = part.begin(); !part(); it = part.current()) {
      tasks.emplace_back(make_task(transform_reduce_fn, it, part.next()));
    }

    auto [f] = schedule(std::move(tasks));

    auto reduce_task = [&] (auto&& futs) mutable {
      return std::transform_reduce(futs.begin(), futs.end(), init, rdc_fn,
                                   [](auto&& fut) mutable { return std::move(fut.get()); });
    };

    return schedule(make_task(reduce_task, std::move(f)));
  }

  template<typename InputIter, typename Fn>
  decltype(auto) for_each(InputIter s, InputIter e, Fn fn) {
    std::for_each(s, e, [&](auto&& x) { enqueue(fn, x); });
    return e;
  }

  ~threadpool();

protected:
  void print(std::ostream&, managed_stop_token);

  // quick shutdown, may not run all tasks
  void stop();

private:
  mutable std::mutex mu_;
  std::condition_variable_any shutdown_cv_;
  std::shared_ptr<managed_stop_source> stop_src_, etc_stop_src_;
  //std::stop_callback<std::function<void()>> stop_cb_;
  task_scheduler scheduler_;
  job_queue jobq_;
  worker_pool worker_pool_, managers_, book_keepers_;
  //std::unique_ptr<signal_handler> sighandler_;
  unsigned max_threads_;

  TP_DISALLOW_COPY_ASSIGN(threadpool)
};

} // namespace thp
#endif /* THREADPOOL_HPP_ */
