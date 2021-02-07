#ifndef WORKER_POOL_HPP_
#define WORKER_POOL_HPP_

#include <algorithm>
#include <mutex>
#include <vector>
#include "include/configuration.hpp"
#include "include/managed_thread.hpp"

namespace thp {

class worker_pool{
public:
  explicit worker_pool(std::shared_ptr<managed_stop_source> src)
      : mu_{}
      , config_{}
      , workers_{}
      , stop_src_{src}
  {}

  template<typename N, typename Fn, typename... Args>
  void start_n_thread(N n, Fn&& fn, Args&&... args) {
    std::unique_lock l(mu_);
    for(size_t i = 0; i < n; ++i)
      create(std::forward<Fn>(fn), std::forward<Args>(args)...);
  }

  template<typename Fn, typename... Args>
  decltype(auto) start_thread(Fn&& fn, Args&&... args) {
    std::unique_lock l(mu_);
    return create(std::forward<Fn>(fn), std::forward<Args>(args)...);
  }

  decltype(auto) size() const {
    std::lock_guard<std::mutex> lck(mu_);
    return workers_.size();
  }

  worker_pool &update_config(const platform::thread_config &cfg) {
    std::lock_guard<std::mutex> lck(mu_);
    config_ = cfg;
    return *this;
  }

  void pause()  { stop_src_->request_pause();  }
  void resume() { stop_src_->request_resume(); }

  void shutdown() {
    if (stop_src_->request_stop())
      for(auto& t : workers_)
        t.second.join();
  }

  ~worker_pool() {
    shutdown();
  }

protected:
  template<typename Fn, typename... Args>
  decltype(auto) create(Fn&& fn, Args&&... args) {
    platform::thread t(stop_src_, std::forward<Fn>(fn), std::forward<Args>(args)...);
    auto ret = t.get_id();
    workers_.emplace(std::make_pair(t.get_id(), std::move(t)));
    return ret;
  }

  void callback(std::thread::id tid) {
    //std::cerr << tid << "logged" << std::endl;
  }

private:
  mutable std::mutex mu_;
  platform::thread_config config_;
  std::unordered_map<std::thread::id, platform::thread> workers_;
  std::shared_ptr<managed_stop_source> stop_src_;
//  std::stop_callback<std::function<void(std::jthread_id)> stop_cb_;
};

} // namespace thp

#endif // WORKER_POOL_HPP_
