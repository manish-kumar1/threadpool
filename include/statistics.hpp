#ifndef STATISTICS_HPP__
#define STATISTICS_HPP__

#include <vector>
#include <unordered_map>
#include <chrono>
#include <thread>

#include "include/configuration.hpp"

namespace thp {

struct workerpool_stats {
  unsigned num_threads;
  unsigned num_workers;
};

struct sched_algo_stats {
  explicit sched_algo_stats()
    : taskq_len{}
    , table{nullptr}
    , num_tasks{0}
  {
    taskq_len.reserve(Static::queue_table_capacity());
  }
  std::vector<size_t> taskq_len;
  std::unordered_map<std::thread::id, int>** table;
  unsigned num_tasks;
};

struct jobq_stats {
  std::chrono::system_clock::time_point ts;
  std::unordered_map<std::thread::id, int> worker_2_q;
  sched_algo_stats algo;
};

struct statistics {
  struct jobq_stats jobq;
  struct workerpool_stats pool;
};

} // namespace thp

#endif // STATISTICS_HPP__
