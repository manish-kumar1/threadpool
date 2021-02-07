#ifndef SIGNAL_HANDLER_HPP_
#define SIGNAL_HANDLER_HPP_

#include <cstring>
#include <functional>
#include <iostream>
#include <signal.h>

#include "include/configuration.hpp"
#include "include/managed_thread.hpp"

namespace thp {
#if 0
class signal_handler : public platform::thread {
public:
  explicit signal_handler(const platform::thread_config &conf)
      : platform::thread(&signal_handler::worker_fn, this) {
    update_config(conf);
  }

  signal_handler(const signal_handler &) = delete;
  signal_handler &operator=(const signal_handler &) = delete;
  signal_handler(signal_handler &&) = delete;

  virtual ~signal_handler() = default;

private:
  void worker_fn(void) {

    int signum = 0;
    for (;;) {
      if (0 == sigwait(get_config()->get_sigset(), &signum)) {
        std::cerr << "[" << native_handle() << "] received signal " << signum
                  << std::endl;
        if (signum == SIGINT)
          break;
        else if (signum == SIGUSR1)
          return;
      }
    }
  }
};
#endif
} // namespace thp

#endif /* SIGNAL_HANDLER_HPP_ */
