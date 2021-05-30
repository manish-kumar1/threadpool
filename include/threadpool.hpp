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

#include "include/partitioner.hpp"
#include "include/algos/partitioner/equal_size.hpp"
#include "include/util.hpp"
#include "include/jobq.hpp"
#include "include/scheduler.hpp"
#include "include/managed_thread.hpp"
#include "include/signal_handler.hpp"
#include "include/task_type.hpp"
#include "include/worker_pool.hpp"
#include "include/task_factory.hpp"
#include "include/all_priority_types.hpp"
#include "include/concepts.hpp"
#include "include/job_type.hpp"

namespace thp {
class threadpool final {
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
  constexpr decltype(auto) schedule(kncpt::ThreadPoolTask auto&&... args) {
    return std::make_tuple(jobq_.schedule_task(args)...);
  }

  template<typename T>
  constexpr decltype(auto) run(job<T>& work) {
    worker_pool workers;
    workers.start_n_thread(work.num_workers(), &job<T>::worker_fn, &work);
    //work.wait();
    std::deque<std::future<typename T::ReturnType>> ret;
    work.futures(std::back_inserter(ret));
    for(auto&& f: ret) f.wait();
    return ret;
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
      using Ret = std::tuple<I, S>;
      using Task = simple_task<Ret>;

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

          std::vector<Task> merge_tasks;
          const auto n = fut_vec.size();
          merge_tasks.reserve(n % 2 != 0 ? (1 + n/2) : n/2);

          for (size_t i = 0; i+1 < n; i += 2)
            merge_tasks.emplace_back(make_task(one_merge, std::move(fut_vec[i]), std::move(fut_vec[i+1])));

          //auto next_fut_vec = jobq_.schedule_task(std::move(merge_tasks));
          auto merge_job = job<Task>(std::move(merge_tasks)).num_workers(std::clamp(n/2, 1lu, 16lu));
          auto next_fut_vec = this->run(merge_job);
          if (n % 2 != 0) {
            next_fut_vec.emplace_back(std::move(fut_vec.back()));
            fut_vec.pop_back();
          }
          //std::ranges::for_each(next_fut_vec, [](auto&& f) { f.get(); });

          return next_fut_vec;
        };

        unsigned part_len = static_cast<unsigned>(len/std::thread::hardware_concurrency());
        auto step = std::clamp(part_len, 2u, Static::stl_sort_cutoff());
        std::vector<Task> tasks;
        tasks.reserve(std::ceil(len/step));
        for(std::size_t i = 0; i < len; i += step)
          tasks.emplace_back(make_task(tp_sort, s+i, s+std::min(i+step, len), comp, proj));

        auto works = job<Task>(std::move(tasks)).num_workers(max_threads_);
        auto futs = this->run(works);
        
        //auto futs = jobq_.schedule_task(std::move(tasks));
        while (futs.size() > 1) {
          futs = level_merge_task(std::move(futs));
        }

        std::ranges::for_each(futs, [](auto&& f) { f.wait(); });
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
    typename PartAlgo = partition::EqualSize<I,S>
  >
  requires std::movable<T>
  constexpr decltype(auto) reduce(I s, S e, T init, BinaryOp rdc_fn, PartAlgo part_algo) {
    return this->transform_reduce(s, e, std::move(init),
                  std::forward<BinaryOp>(rdc_fn),
                  std::identity(),
                  std::forward<PartAlgo>(part_algo));
  }

  template <
    std::input_iterator I, std::sentinel_for<I> S,
    typename T,
    typename BinaryOp,
    typename UnaryOp = std::identity,
    typename PartAlgo = partition::EqualSize<I,S>
  >
  requires std::movable<T>
  constexpr decltype(auto) transform_reduce(I s, S e, T init,
                                  BinaryOp rdc_fn, UnaryOp tr_fn,
                                  PartAlgo part_algo) {
    auto transform_reduce_fn = [=](auto&& subrng) {
      return std::transform_reduce(subrng.begin(), subrng.end(), init, rdc_fn, tr_fn);
    };

    return enqueue([=, this] () mutable {
      std::vector<simple_task<T>> tasks;
      partitioner partitions(part_algo);
      tasks.reserve(partitions.count());

      for(auto&& sr : partitions) tasks.emplace_back(make_task(transform_reduce_fn, sr));

      //auto futs = jobq_.schedule_task(std::move(tasks));
      auto work = job<simple_task<T>>(std::move(tasks)).num_workers(std::clamp(partitions.count(), 1lu, 24lu));
      auto futs = this->run(work);
      return std::transform_reduce(futs.begin(), futs.end(), init, rdc_fn,
                                   [](auto&& fut) mutable { return std::move(fut.get()); });
    });
  }

  template<std::input_iterator I, std::sentinel_for<I> S, typename Fn>
  constexpr decltype(auto) for_each(I s, S e, Fn fn) {
    return std::ranges::for_each(s, e, [fn, this](auto&& x) { return enqueue(fn, x); });
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
