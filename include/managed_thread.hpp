#ifndef MANAGED_THREAD_HPP_
#define MANAGED_THREAD_HPP_

#include <algorithm>
#include <functional>
#include <thread>

#include "include/configuration.hpp"

namespace thp {
// interface
struct managed_thread {
  virtual bool joinable() noexcept = 0;
  virtual std::thread::id get_id() const noexcept = 0;
  virtual std::thread::native_handle_type native_handle() = 0;

  virtual void join() = 0;
  virtual void stop() = 0;
  virtual void start() = 0;
  virtual void pause() = 0;

  virtual std::shared_ptr<configuration> get_config() = 0;

  virtual ~managed_thread(){};
};

namespace platform {

// a unit of worker
class thread : public thp::managed_thread {
public:
  template <typename Fn, typename... Args>
  explicit thread(Fn &&fn, Args &&... args)
      : config_{nullptr},
        th_(std::forward<Fn>(fn), std::forward<Args>(args)...) {}

  thread(thread &&) noexcept = default;
  thread &operator=(thread &&) noexcept = default;

  thread(const thread &) = delete;
  thread &operator=(const thread &) = delete;

  bool joinable() noexcept override                { return th_.joinable(); }
  std::thread::id get_id() const noexcept override { return th_.get_id(); }
  std::thread::native_handle_type native_handle() override { return th_.native_handle(); }
  void join() override { if (joinable()) th_.join(); }

  void stop() override { config_->cancel(this); }
  void start() override {}
  void pause() override {}

  void update_config(const thread_config &new_config) {
    if (!config_)
      config_.reset(new thread_config(new_config));

    config_->apply(this);
  }

  std::shared_ptr<configuration> get_config() override {
    if (!config_) config_.reset(new thread_config());
    config_->retrieve(this);
    return config_;
  }

  virtual ~thread() {
    join();
  }

private:
  std::shared_ptr<configuration> config_;
  std::thread th_;
};

} // namespace platform
} // namespace thp

#endif /* MANAGED_THREAD_HPP_ */
