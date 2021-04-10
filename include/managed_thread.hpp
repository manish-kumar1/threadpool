#ifndef MANAGED_THREAD_HPP_
#define MANAGED_THREAD_HPP_

#include <algorithm>
#include <functional>
#include <thread>
#include <shared_mutex>

#include "include/configuration.hpp"
#include "include/managed_stop_source.hpp"

namespace thp {
// interface
struct managed_thread {
  virtual bool joinable() noexcept = 0;
  virtual std::thread::id get_id() const noexcept = 0;
  virtual std::thread::native_handle_type native_handle() = 0;

  virtual void join() = 0;
  virtual void stop() = 0;
  virtual void resume() = 0;
  virtual void pause() = 0;

  virtual std::shared_ptr<configuration> get_config() = 0;

  virtual ~managed_thread() = default;
};

namespace platform {

// a unit of worker
class thread final : public thp::managed_thread {
public:
  template <typename Fn, typename... Args, typename =
		  std::enable_if_t<std::is_invocable_v<Fn, Args...>>>
  explicit thread(Fn &&fn, Args &&... args)
  : src_{std::make_shared<managed_stop_source>()}
  , config_{nullptr}
  , th_{create(std::forward<Fn>(fn), std::forward<Args>(args)...)}
  {}

  template <typename Fn, typename... Args>
  explicit thread(std::shared_ptr<managed_stop_source> stop_src, Fn &&fn, Args &&... args)
  : src_{stop_src}
  , config_{nullptr}
  , th_{create(std::forward<Fn>(fn), std::forward<Args>(args)...)}
  {}

  thread(thread &&) noexcept = default;
  thread &operator=(thread &&) noexcept = default;

  thread(const thread &) = delete;
  thread &operator=(const thread &) = delete;

  bool joinable() noexcept override                        { return th_.joinable(); }
  std::thread::id get_id() const noexcept override         { return th_.get_id();   }
  std::thread::native_handle_type native_handle() override { return th_.native_handle(); }
  void join() override { if (joinable()) th_.join(); }

  void stop() override   { src_->request_stop();   }
  void resume() override { src_->request_resume(); }
  void pause() override  { src_->request_pause();  }

  void update_config(const thread_config &new_config) {
    if (!config_)
      config_ = std::make_shared<thread_config>(new_config);

    config_->apply(this);
  }

  std::shared_ptr<configuration> get_config() override {
    if (!config_) config_ = std::make_shared<thread_config>();
    config_->retrieve(this);
    return config_;
  }

  virtual ~thread() {
    try {
      join();
    }
    catch(...) {}
  }

protected:
  template<typename Fn, typename... Args>
  constexpr std::thread create(Fn&& fn, Args&&... args) {
    using Tup = std::tuple<Args...>;
    using LastType = std::tuple_element_t<std::tuple_size_v<Tup> - 1, Tup>;
    if constexpr (std::is_same_v<LastType, managed_stop_token>)
      return std::thread(std::forward<Fn>(fn), std::forward<Args>(args)...);
    else
      return std::thread(std::forward<Fn>(fn), std::forward<Args>(args)..., src_->get_managed_token());
  }

private:
  std::shared_ptr<managed_stop_source> src_;
  std::shared_ptr<configuration> config_;
  std::thread th_;
};

} // namespace platform
} // namespace thp

#endif /* MANAGED_THREAD_HPP_ */
