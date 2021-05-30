#ifndef JOB_TYPE_HPP__
#define JOB_TYPE_HPP__

#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>

#include "include/task_type.hpp"
//#include "include/concepts.hpp"
#include "include/managed_stop_token.hpp"

namespace thp {
//
// TODO: job is an iterable graph, where each node is a computational point(task)
// e.g. rng::for_each(job, [](auto&& task) { task.execute(); });
// non-cyclic graph, where sequential operation will be queued, while parallel operations
// can be queued in group and executed parallely by tp
//
// Define functional construct to build graph topology 
//

template<typename TaskType, typename TaskContainer = std::vector<TaskType>>
struct job {
  constexpr explicit job(std::vector<TaskType>&& data)
  : mu{}
  //, knt{0}
  , workers{0}
  //, wait_cv{}
  , tasks{std::move(data)}
  {}

  job(job&& rhs) {
    std::unique_lock lk(mu, std::defer_lock), rhs_lk(rhs.mu, std::defer_lock);
    std::lock(lk, rhs_lk);
    //std::swap(idx, rhs.idx);
    std::swap(workers, rhs.workers);
    std::swap(tasks, rhs.tasks);
  }

  job& operator = (job&& rhs) {
    std::unique_lock lk(mu, std::defer_lock), rhs_lk(rhs.mu, std::defer_lock);
    std::lock(lk, rhs_lk);
    idx = rhs.idx;
    workers = rhs.workers;
    tasks = std::move(rhs.workers);
    return *this;
  }

  job(const job&) = delete;
  job& operator = (const job&) = delete;
  constexpr ~job() = default;

  constexpr job& on_cpu(size_t cpuid) { return std::move(*this); }
  constexpr job& on_node(size_t nodeid) { return std::move(*this); }

  template<typename Duration>
  constexpr job& run_for(Duration&& dur) { return std::move(*this); }

  template<typename TimePoint>
  constexpr job& run_until(TimePoint&& tp) { return std::move(*this); }

  constexpr decltype(auto) num_workers(std::size_t w) {
    workers = w;
    return std::move(*this);
  }
  constexpr std::size_t num_workers() const { return workers; }

  decltype(auto) worker_fn(managed_stop_token st) {
    const auto n = tasks.size();
    const thread_local auto my_idx = std::atomic_fetch_add_explicit(&idx, 1, std::memory_order::acq_rel);
    for(auto i = my_idx; i < n; i += workers) {
      {
        if (st.stop_requested()) [[unlikely]] break;
        //std::cerr << std::this_thread::get_id() << "work[" << i << "] " << idx << " " << tasks.size() << "\n";
      }
      tasks[i].execute();
    }
    //std::atomic_fetch_add_explicit(&knt, 1, std::memory_order::acq_rel);
    //wait_cv.notify_one();
  }

  template<typename Oiter>
  constexpr decltype(auto) futures(Oiter o) {
    return std::ranges::transform(tasks, o, [](auto&& t) { return t.future(); });
  }

  constexpr void wait() {
    //std::unique_lock l(mu);
    //wait_cv.wait(l, [&] { return knt.load(std::memory_order::relaxed) == idx.load(std::memory_order::relaxed); });
  }

private:
  //boost::adjacency_list<TaskType, VecS, VecS> tasks;
  mutable std::mutex mu;
  std::atomic<int> idx;
  std::size_t workers;
  //std::condition_variable_any wait_cv;
  TaskContainer tasks;
};


} // namespace thp

#endif // JOB_TYPE_HPP__
