#ifndef STATISTICS_HPP__
#define STATISTICS_HPP__

#include <array>
#include <deque>
#include <memory>
#include <chrono>

#include "include/configuration.hpp"
#include "include/task_queue.hpp"
#include "include/executable.hpp"
#include "include/all_priority_types.hpp"

namespace thp {
//namespace stats {

struct workerpool_stats {
  std::size_t num_threads;
  std::size_t num_workers;
};

struct outputs {
  std::deque<std::unique_ptr<executable>>* cur_output;
  std::size_t new_tasks;

  outputs& reset() {
    new_tasks = 0;
    return *this;
  }
};

struct inputs {
  std::vector<task_queue*>& qs;
  std::size_t num_tasks;
  int load_factor;
};

struct jobq_stats {
  const inputs in;
  outputs out;
};

struct statistics {
  std::chrono::system_clock::time_point ts;
  struct jobq_stats jobq;
  struct workerpool_stats pool;
} __attribute__((aligned(4)));

//} // namespace stats
} // namespace thp

#endif // STATISTICS_HPP__
