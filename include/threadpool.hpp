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

namespace thp {
class threadpool {
public:
  explicit threadpool(unsigned max_threads = std::thread::hardware_concurrency());

  // waits till condition of no tasks is statisfied
  void drain();

  // graceful shutdown
  void shutdown();

  // could be heterogeneous task types
  template <typename... Args>
  constexpr decltype(auto) schedule(Args &&... args) {
    return jobq_.insert(args...);
  }

  template <typename Fn, typename... Args>
  constexpr decltype(auto) enqueue(Fn &&fn, Args &&... args) {
    using Ret = std::invoke_result_t<Fn, Args...>;
    return schedule(util::make_task<Ret>(std::forward<Fn>(fn), std::forward<Args>(args)...));
  }

  // parallel algorithm, for benchmarks see examples/reduce.cpp
  template <typename InputIter, typename T, typename BinaryOp, typename Partitioner>
  decltype(auto) reduce(InputIter s, InputIter e, T init, BinaryOp rdc_fn, Partitioner part) {
    return transform_reduce(s, e, std::move(init), rdc_fn, std::identity(), part);
  }

  template <typename InputIter, typename T, typename BinaryOp, typename UnaryOp,
            typename Partitioner>
  decltype(auto) transform_reduce(InputIter s, InputIter e, T init,
                                  BinaryOp rdc_fn, UnaryOp tr_fn,
                                  Partitioner part) {
    auto transform_reduce_fn = [=](auto &&s1, auto &&e1) {
      return std::transform_reduce(s1, e1, init, rdc_fn, tr_fn);
    };

    auto reduce_task = [&]() {
      std::vector<std::unique_ptr<simple_task<T>>> tasks;
      tasks.reserve(part.count());

      for (auto it = part.begin(); !part(); it = part.current()) {
        tasks.emplace_back(util::make_task<T>(transform_reduce_fn, it, part.next()));
      }

      auto futs = schedule(std::move(tasks));

      return std::transform_reduce(futs.begin(), futs.end(), init, rdc_fn,
                                   [](auto&& f) { return std::move(f.get());});
    };

    return schedule(util::make_task<T>(reduce_task));
  }

  template<typename InputIter, typename Fn>
  decltype(auto) for_each(InputIter s, InputIter e, Fn fn) {
    for(auto it = s; it != e; ++it) {
      enqueue(fn, *it);
    }
    return e;
  }

  ~threadpool();

protected:
  void print(std::stop_token, std::ostream&);

  // quick shutdown, may not run all tasks
  void stop();

private:
  mutable std::mutex mu_;
  std::condition_variable_any shutdown_cv_;
  std::stop_source stop_src_;
  std::stop_callback<std::function<void()>> stop_cb_;
  job_queue jobq_;
  task_scheduler scheduler_;
  worker_pool pool_;
//  std::unique_ptr<signal_handler> sighandler_;
  unsigned max_threads_;

  TP_DISALLOW_COPY_ASSIGN(threadpool);
};

} // namespace thp
#endif /* THREADPOOL_HPP_ */
