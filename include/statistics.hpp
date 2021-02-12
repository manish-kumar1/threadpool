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

struct workerpool_stats {
  std::size_t num_threads;
  std::size_t num_workers;
};

struct outputs {
  std::deque<std::unique_ptr<executable>>& output;
  std::size_t new_tasks;
};

struct inputs {
  const std::vector<task_queue*>& qs;
  const int num_tasks;
  const int load_factor;
};

struct jobq_stats {
  std::chrono::system_clock::time_point ts;
  const inputs in;
  outputs out;
};

struct statistics {
  struct jobq_stats jobq;
  struct workerpool_stats pool;
};

} // namespace thp

#endif // STATISTICS_HPP__
