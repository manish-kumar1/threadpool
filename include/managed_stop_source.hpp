#ifndef MANAGED_STOP_SOURCE_HPP__
#define MANAGED_STOP_SOURCE_HPP__

#include <stop_token>
#include <shared_mutex>
#include <condition_variable>
#include <iostream>
namespace thp {

// fwd declaration
struct managed_stop_token;

// implementation class
struct managed_stop_source_impl
{
  friend class managed_stop_token;

  bool request_pause() noexcept {
    std::unique_lock l(mu);
    paused = true;
    return paused;
  }
  
  void request_resume() noexcept {
    {
      std::unique_lock l(mu);
      paused = false;
    }
    cond.notify_all();
  }

  bool pause_requested() const {
    std::shared_lock l(mu);
    return paused;
  }

  void pause() {
	  std::unique_lock l(mu);
	  cond.wait(l, [&] { return !paused; });
  }

protected:
  mutable std::shared_mutex mu;
  bool paused;
  std::condition_variable_any cond;
};

struct managed_stop_source : std::stop_source, std::enable_shared_from_this<managed_stop_source> {
	managed_stop_source()
	: std::stop_source()
  , std::enable_shared_from_this<managed_stop_source>()
	, impl{std::make_shared<managed_stop_source_impl>()}
	{
    //std::cerr << "managed_stop_source(): " << impl.get() << std::endl;
  }

	managed_stop_source(const managed_stop_source& rhs)
	: std::stop_source{rhs}
  , std::enable_shared_from_this<managed_stop_source>()
	, impl{rhs.impl}
	{}

	managed_stop_source& operator = (const managed_stop_source& rhs)
	{
		impl = rhs.impl;
		return *this;
	}

	managed_stop_source(managed_stop_source&& rhs)
	: std::stop_source{rhs}
	, impl{std::move(rhs.impl)}
	{
	}

	managed_stop_source& operator=(managed_stop_source&& rhs) {
		impl = std::move(rhs.impl);
		return (*this);
	}

  friend std::ostream& operator << (std::ostream& oss, const managed_stop_source& src) {
    oss << "managed_stop_source: " << &src << "::" << src.impl.get() << "::" << src.impl.use_count();
    return oss;
  }

	bool request_pause() noexcept {
		return impl->request_pause();
	}

	[[nodiscard]] managed_stop_token get_managed_token()  noexcept;

	void request_resume() noexcept {
		impl->request_resume();
	}

	bool pause_requested() {
		return impl->pause_requested();
	}

	void pause() {
    impl->request_pause();
		impl->pause();
	}
protected:
  std::shared_ptr<managed_stop_source_impl> impl;
};

} // namespace thp

#endif // MANAGED_STOP_SOURCE_HPP__

