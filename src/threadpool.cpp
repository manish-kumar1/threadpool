#include <condition_variable>
#include <future>
#include <memory>
#include <thread>
#include <glog/logging.h>
#include <iomanip>
#include <ctime>
#include <iostream>

#include "include/threadpool.hpp"
#include "include/worker_pool.hpp"

namespace thp {

threadpool::threadpool(unsigned max_threads)
  : mu_{}
  , shutdown_cv_{}
  , stop_src_{}
  , stop_cb_{stop_src_.get_token(), [this] { stop(); }}
  , jobq_{}
  , scheduler_{}
  , pool_{}
  , max_threads_{std::max(2u, max_threads)}
{
  std::lock_guard<std::mutex> lck(mu_);
  auto sched_conf = platform::thread_config();
  //sched_conf.set_affinity({0});

  for (size_t i = 0; i < max_threads_; ++i)
    pool_.start_thread(&job_queue::worker_fn, &jobq_, stop_src_.get_token());

  pool_.start_thread(&job_queue::schedule_fn, &jobq_, &scheduler_, stop_src_.get_token()).update_config(sched_conf);
  //pool_.start_thread(&threadpool::print, this, stop_src_.get_token(), std::ref(std::cerr));
}

void threadpool::print(std::stop_token st, std::ostream& oss) {
  statistics stats;
  std::vector<unsigned> worker_utilization;

  for (;;) {
    std::unique_lock l(mu_);
    shutdown_cv_.wait_for(l, Static::stats_collection_period());
    if (st.stop_requested())
      break;

    l.unlock();

    jobq_.copy_stats(stats.jobq);

    worker_utilization.clear();
    worker_utilization.resize(stats.jobq.algo.taskq_len.size(), 0);
    auto start = std::chrono::system_clock::to_time_t(stats.jobq.ts);

    oss << "job queue stats at: " << std::asctime(std::localtime(&start));
    oss << "total tasks: " << stats.jobq.algo.num_tasks << '\n';
    //for(auto& p : stats.jobq.worker_2_q)
    //  oss << p.first << " : "  << p.second << '\n';
    for(auto& w : stats.jobq.worker_2_q) {
      ++worker_utilization[w.second];
      oss << w.second << ", ";
    }
    oss << "\nQueue Length Worker\n";
    for(unsigned i = 0; i < worker_utilization.size(); ++i)
      oss << std::setw(5) << i << " " << std::setw(6) << stats.jobq.algo.taskq_len[i]
          << " " << worker_utilization[i] << '\n';

    oss << std::endl;
  }
}

void threadpool::drain() { jobq_.drain(); }

void threadpool::shutdown() {
  drain();
  stop_src_.request_stop();
}

void threadpool::stop() {
  jobq_.close();  
  jobq_.stop();
  pool_.shutdown();
}

threadpool::~threadpool() {
  shutdown();
}

} // namespace thp
