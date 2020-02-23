#include <iostream>
#include <memory>
#include <pthread.h>
#include <sstream>
#include <string>
#include <vector>

#include <signal.h>

#include "include/configuration.hpp"
#include "include/managed_thread.hpp"

#include "platform/thread_config.h"

namespace thp {
namespace platform {

thread_config::thread_config() { conf_ = config_get_thread_config(); }

thread_config::thread_config(const thread_config &rhs) {
  conf_ = config_get_thread_config();
  *conf_ = *rhs.conf_;
}

thread_config &thread_config::operator=(const thread_config &rhs) {
  if (!conf_)
    conf_ = config_get_thread_config();

  *conf_ = *rhs.conf_;
  return *this;
}

int thread_config::apply(managed_thread *th) {
  auto hndl = th->native_handle();
  return config_apply(hndl, conf_);
}

int thread_config::retrieve(managed_thread *th) {
  auto hndl = th->native_handle();
  return config_retrieve(hndl, conf_);
}

configuration &thread_config::set_priority(int priority) {
  config_set_priority(conf_, priority);
  return *this;
}

configuration &thread_config::set_policy(int policy) {
  config_set_policy(conf_, policy);
  return *this;
}

configuration &thread_config::set_affinity(std::vector<unsigned> cpuid) {
  int arr[64];
  std::copy(cpuid.begin(), cpuid.end(), arr);
  config_set_affinity(conf_, arr, cpuid.size());
  return *this;
}

configuration &thread_config::unblock_signals(std::vector<int> sig) {
  config_unblock_signals(conf_, sig.data(), sig.size());
  return *this;
}

const sigset_t *thread_config::get_sigset() const {
  return config_get_sigset(conf_);
}

int thread_config::cancel(managed_thread *th) {
  return config_cancel_thread(th->native_handle());
}

void thread_config::display(managed_thread * /*th*/) {}

thread_config::~thread_config() { config_remove_thread_config(conf_); }

} // namespace platform
} // namespace thp
