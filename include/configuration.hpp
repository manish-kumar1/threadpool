#ifndef CONFIGURATION_HPP__
#define CONFIGURATION_HPP__

#include <memory>
#include <pthread.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <chrono>

#include "platform/thread_config.h"

namespace thp {
struct managed_thread;

struct configuration {
  virtual int apply(managed_thread *th) = 0;
  virtual void display(managed_thread *th) = 0;
  virtual int retrieve(managed_thread *th) = 0;
  virtual int cancel(managed_thread *th) = 0;

  virtual configuration &set_priority(int priority) = 0;
  virtual configuration &set_policy(int policy) = 0;
  virtual configuration &set_affinity(std::vector<unsigned> cpuid) = 0;
  virtual configuration &unblock_signals(std::vector<int> sig) = 0;

  virtual const sigset_t *get_sigset() const = 0;
  virtual ~configuration() {}
};

namespace platform {
class thread_config : public configuration {
public:
  explicit thread_config();
  thread_config(const thread_config &rhs);
  thread_config &operator=(const thread_config &rhs);

  configuration &set_priority(int priority) override;
  configuration &set_policy(int policy) override;
  configuration &set_affinity(std::vector<unsigned> cpuid) override;
  configuration &unblock_signals(std::vector<int> sig) override;
  const sigset_t *get_sigset() const override;

  int cancel(managed_thread *th) override;
  void display(managed_thread *th) override;
  int apply(managed_thread *th) override;
  int retrieve(managed_thread *th) override;
  virtual ~thread_config();

private:
  struct config *conf_;
};

} // namespace platform

namespace Static {
  constexpr inline const decltype(auto) shutdown_grace_period()    { return std::chrono::milliseconds(2000);}
  constexpr inline const decltype(auto) stats_collection_period()  { return std::chrono::milliseconds(1000); }
  constexpr inline const decltype(auto) schedule_request_timeout() { return std::chrono::milliseconds(10); }
  constexpr inline const decltype(auto) scheduler_tick()           { return std::chrono::microseconds(60);  }
  constexpr inline const decltype(auto) per_queue_capacity()       { return 16*1024;                        }
  constexpr inline const decltype(auto) queue_table_capacity()     { return 1024;                           }
} // namespace Static

} // namespace thp

#endif // CONFIGURATION_HPP__
