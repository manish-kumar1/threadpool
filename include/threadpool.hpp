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
  constexpr I sort(I s, S e, Comp comp = {}, Proj proj = {}) {
    unsigned len = std::ranges::distance(s, e);
    //std::cerr << "sort:(" << len << ")" << std::endl;
    if (len <= 1000000u)
        std::ranges::sort(s, e, comp, proj);
    else {
        auto step = std::clamp(len/num_workers(), 100000u, 1000000u);
        using len_t = decltype(len);

        unsigned total_partitions = std::ceil(len/step);
        std::vector<std::shared_ptr<simple_task<I>>> tasks;
        tasks.reserve(total_partitions);
        for (len_t i = 0; i < len; i += step) {
          tasks.emplace_back(make_task(&threadpool::sort<I, S, Comp, Proj>, this, s+i, s+std::min(i+step, len), comp, proj));
        }

        //std::cerr << "total partition: " << tasks.size() << std::endl;
        auto [futs] = schedule(std::move(tasks));
        std::ranges::for_each(futs, [](auto&& f) { f.get(); });
        // merge
        len_t range_size = 0;
        for(range_size = step; range_size < len; range_size *= 2) {
          std::vector<std::shared_ptr<simple_task<I>>> merge_tasks;
          merge_tasks.reserve(1+ (len-1)/(2*range_size));
          //merge_tasks.reserve(std::ceil(total_partitions/(2*two)));
          for (len_t i = 0; i < len; i += 2*range_size) {
            auto ii = i;
            auto mm = std::min(len, ii+range_size);
            auto ll = std::min(len, mm+range_size);
            //std::cerr << ii << ", " << mm << ", " << ll << ", " << range_size << std::endl;
            merge_tasks.emplace_back(make_task(std::ranges::inplace_merge, s+ii, s+mm, s+ll, comp));
          }
          //std::cerr << std::endl;
          auto [mf] = schedule(std::move(merge_tasks));
          std::ranges::for_each(mf, [](auto&& f) { f.get(); });
          //std::ranges::copy(s, e, std::ostream_iterator<decltype(*s)>(std::cerr, ", ")); std::cerr << std::endl;
        }

        if (total_partitions % 2 != 0) {
          std::ranges::inplace_merge(s, std::next(s, range_size/2), e, comp);
        }
    }
    return s;
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

  auto num_workers() const { return max_threads_; }

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
