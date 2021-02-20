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
#include "include/managed_stop_token.hpp"

namespace thp {

threadpool::threadpool(unsigned max_threads)
  : mu_{}
  , shutdown_cv_{}
  , jobq_{}
  , worker_pool_{}
  , managers_{}
  , book_keepers_{}
  , max_threads_{max_threads}
{
  std::lock_guard<std::mutex> lck(mu_);
  auto worker_conf = platform::thread_config();

  worker_pool_.update_config(worker_conf);
  worker_pool_.start_n_thread(max_threads_, &job_queue<TaskQueueTupleType>::worker_fn, &jobq_);

  managers_.start_thread(&job_queue<TaskQueueTupleType>::schedule_fn, &jobq_);
  //book_keepers_.start_thread(&threadpool::print, this, std::ref(std::cerr));
}

void threadpool::print(std::ostream& oss, managed_stop_token st) {
//  statistics stats;
  std::vector<unsigned> worker_utilization;

  for (;;) {
    std::unique_lock l(mu_);
    shutdown_cv_.wait_for(l, Static::stats_collection_period());
    if (st.stop_requested())
      break;
    if (st.pause_requested()) {
      l.unlock();
      st.pause();
      l.lock();
    }
#if 0
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
    oss << "job_queue stats: " << std::endl;
#endif
  }
}

void threadpool::drain() { jobq_.drain(); }

void threadpool::shutdown() {
  worker_pool_.shutdown();
  managers_.shutdown();
  book_keepers_.shutdown();
}

void threadpool::pause() {
	//jobq_.close();
	worker_pool_.pause();
	managers_.pause();
}

void threadpool::resume() {
  worker_pool_.resume();
  managers_.resume();
}

void threadpool::stop() {
  jobq_.close();  
  jobq_.stop();
  worker_pool_.shutdown();
  managers_.shutdown();
  book_keepers_.shutdown();
}

threadpool::~threadpool() {
  shutdown();
}

} // namespace thp
