#ifndef PRIORITY_TASKQ_HPP_
#define PRIORITY_TASKQ_HPP_

#include <algorithm>
#include <functional>

#include "include/configuration.hpp"
#include "include/task_queue.hpp"
#include "include/task_type.hpp"

namespace thp {

class priority_taskq : public task_queue {
public:
  void put(std::unique_ptr<executable>&& t) {
    std::lock_guard l(mu_);
    _put(std::move(t));
    std::push_heap(tasks.begin(), tasks.end()); 
  }

  bool pop(std::unique_ptr<executable>& t) override {
    std::unique_lock l(mu_);
    std::pop_heap(tasks.begin(), tasks.end());
    return _pop(t);
  }

  virtual ~priority_taskq() = default;
};

} // namespace thp

#endif // TASKQ_HPP_
