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
  unsigned num_threads;
  unsigned num_workers;
};

struct outputs {
  std::deque<std::unique_ptr<executable>>& output;
  unsigned new_tasks;
};

struct inputs {
  const std::vector<task_queue*>& qs;
  int num_tasks;
};

struct jobq_stats {
  std::chrono::system_clock::time_point ts;
  inputs in;
  outputs out;
};

struct statistics {
  struct jobq_stats jobq;
  struct workerpool_stats pool;
};

} // namespace thp

#endif // STATISTICS_HPP__
