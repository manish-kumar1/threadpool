#ifndef TASK_QUEUE_HPP_
#define TASK_QUEUE_HPP_

#include <deque>
#include <exception>
#include <memory>
#include <shared_mutex>

#include "include/task_type.hpp"

namespace thp {

// one task container queue
class task_queue {
public:
  virtual void put(std::unique_ptr<task>&& p) {
    std::unique_lock l(mu_);
    _put(std::move(p));
  }

  virtual bool pop(std::unique_ptr<task>& t) {
    std::unique_lock l(mu_);
    return _pop(t);
  }

  template<typename InputIter>
  task_queue& insert(InputIter s, InputIter e) {
    std::unique_lock l(mu_);
    tasks.insert(tasks.end(), s, e);
    return *this;
  }

  unsigned len() const {
    std::shared_lock l(mu_);
    return tasks.size();
  }

  bool empty() const { return 0 == len(); }

protected:
  void _put(std::unique_ptr<task>&& p) {
    tasks.emplace_back(std::move(p));
  }

  bool _pop(std::unique_ptr<task>& t) {
    bool ret = false;
    if (!tasks.empty()) {
      t = std::move(tasks.back());
      tasks.pop_back();
      ret = true;
    }
    return ret;
  }

  mutable std::shared_mutex mu_;
  std::deque<std::unique_ptr<task>> tasks;
};

} // namespace thp

#endif // TASK_QUEUE_HPP_
