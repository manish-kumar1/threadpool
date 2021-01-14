#ifndef WORKER_POOL_HPP_
#define WORKER_POOL_HPP_

#include <algorithm>
#include <mutex>
#include <vector>
#include "include/configuration.hpp"
#include "include/managed_thread.hpp"

namespace thp {

class worker_pool {
public:
  explicit worker_pool()
      : mu_{}
      , config_{}
      , workers_{} {
  }

  template<typename N, typename Fn, typename... Args>
  void start_n_thread(N n, Fn&& fn, Args&&... args) {
    std::unique_lock l(mu_);
    std::generate_n(std::back_inserter(workers_), n,
      [&]() { return platform::thread(
                      std::forward<Fn>(fn), 
                      std::forward<Args>(args)...); 
            });
  }

  template<typename Fn, typename... Args>
  decltype(auto) start_thread(Fn&& fn, Args&&... args) {
    std::unique_lock l(mu_);
    return workers_.emplace_back(platform::thread(
                      std::forward<Fn>(fn), 
                      std::forward<Args>(args)...));
  }

  decltype(auto) size() {
    std::lock_guard<std::mutex> lck(mu_);
    return workers_.size();
  }

  worker_pool &update_config(const platform::thread_config &cfg) {
    std::lock_guard<std::mutex> lck(mu_);
    config_ = cfg;
    return *this;
  }

  void shutdown() {
    std::unique_lock l(mu_);
    for(auto& t : workers_)
      t.join();
  }

  ~worker_pool() = default;

private:
  mutable std::mutex mu_;
  platform::thread_config config_;
  std::vector<platform::thread> workers_;
};

} // namespace thp

#endif // WORKER_POOL_HPP_
