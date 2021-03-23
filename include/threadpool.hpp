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
#include <algorithm>
#include <cmath>
#include <cassert>

#include "include/util.hpp"
#include "include/jobq.hpp"
#include "include/scheduler.hpp"
#include "include/managed_thread.hpp"
#include "include/signal_handler.hpp"
#include "include/task_type.hpp"
#include "include/worker_pool.hpp"
#include "include/task_factory.hpp"
#include "include/all_priority_types.hpp"

namespace thp {

class threadpool {
  using TaskQueueTupleType = TaskQueueTuple<AllPriorityTupleType>::type;

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
    auto futs = std::make_tuple(jobq_.schedule_task(args)...);
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

  template <std::random_access_iterator I, std::sentinel_for<I> S,
            typename Comp = std::ranges::less, typename Proj = std::identity>
  requires std::sortable<I, Comp, Proj>
  constexpr decltype(auto) sort(I start, S end, Comp cmp = {}, Proj prj = {}) {

    std::function<std::tuple<I,S>(I, S, Comp, Proj)> tp_sort = [this, &tp_sort](I s, S e, Comp comp, Proj proj) {
      const std::size_t len = std::ranges::distance(s, e);
      if (len <= Static::stl_sort_cutoff()) {
        std::ranges::sort(s, e, comp, proj);
      }
      else {
        // level merge of sorted ranges
        auto level_merge_task = [this] (auto&& fut_vec) mutable {
          //std::cerr << "level_merge_task: " << fut_vec.size() << std::endl;
          auto one_merge = [](auto&& f1, auto&& f2) mutable {
            auto [s1, e1] = f1.get();
            auto [s2, e2] = f2.get();
            assert(e1 == s2);
            std::ranges::inplace_merge(s1, s2, e2);
            return std::make_tuple(s1, e2);
          };

          std::vector<std::unique_ptr<simple_task<std::tuple<I, S>>>> merge_tasks;
          const auto n = fut_vec.size();
          merge_tasks.reserve(n % 2 != 0 ? (1 + n/2) : n/2);

          for (size_t i = 0; i+1 < n; i += 2)
            merge_tasks.emplace_back(make_task(one_merge, std::move(fut_vec[i]), std::move(fut_vec[i+1])));

          auto next_fut_vec = jobq_.schedule_task(std::move(merge_tasks));
          if (n % 2 != 0) {
            next_fut_vec.emplace_back(std::move(fut_vec.back()));
            fut_vec.pop_back();
          }
          //std::ranges::for_each(next_fut_vec, [](auto&& f) { f.get(); });

          return next_fut_vec;
        };

        unsigned part_len = static_cast<unsigned>(len/std::thread::hardware_concurrency());
        auto step = std::clamp(part_len, 2u, Static::stl_sort_cutoff());
        std::vector<std::unique_ptr<simple_task<std::tuple<I, S>>>> tasks;
        tasks.reserve(std::ceil(len/step));
        for(std::size_t i = 0; i < len; i += step)
          tasks.emplace_back(make_task(tp_sort, s+i, s+std::min(i+step, len), comp, proj));

        auto futs = jobq_.schedule_task(std::move(tasks));
        while (futs.size() > 1) {
          futs = level_merge_task(std::move(futs));
        }

        std::ranges::for_each(futs, [](auto&& f) { f.get(); });
      }
      return std::make_tuple(s, e);
    };

    return enqueue(tp_sort, std::forward<I>(start), std::forward<S>(end),
                            std::forward<Comp>(cmp), std::forward<Proj>(prj));
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

    std::vector<std::unique_ptr<simple_task<T>>> tasks;
    tasks.reserve(part.count());

    for (auto it = part.begin(); !part(); it = part.current()) {
      tasks.emplace_back(make_task(transform_reduce_fn, it, part.next()));
    }

    auto f = jobq_.schedule_task(std::move(tasks));

    return enqueue([&, futs = std::move(f)] () mutable {
      return std::transform_reduce(futs.begin(), futs.end(), init, rdc_fn,
                                   [](auto&& fut) mutable { return std::move(fut.get()); });
    });
  }

  std::size_t size() const {
    return jobq_.size();
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
  job_queue<TaskQueueTupleType> jobq_;
  worker_pool worker_pool_, managers_, book_keepers_;
  //std::unique_ptr<signal_handler> sighandler_;
  unsigned max_threads_;

  TP_DISALLOW_COPY_ASSIGN(threadpool)
};

} // namespace thp
#endif /* THREADPOOL_HPP_ */
