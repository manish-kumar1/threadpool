#ifndef TASK_QUEUE_HPP_
#define TASK_QUEUE_HPP_

#include <deque>
#include <memory>
#include <shared_mutex>

#include "include/task_type.hpp"
#include "include/traits.hpp"

namespace thp {

// task queue interface
struct task_queue {
  virtual bool pop(std::unique_ptr<executable>&) = 0;
  virtual std::size_t len() const = 0;
  virtual bool empty() { return 0u == len(); }
  virtual ~task_queue() = default;
};

namespace details {
// one executable container queue
// class with mutex, implement custom move constructors, as
// they are defaulted to delete
template<typename Ret, typename Prio=void>
class priority_taskq : public task_queue {
public:
  using PriorityTaskType = priority_task<Ret, Prio>;

  explicit priority_taskq()
  : mu{}
  , tasks{}
  {}

  priority_taskq(priority_taskq&& rhs)
  : mu{}
  , tasks{}
  {
    std::lock_guard lk(rhs.mu);
    tasks = std::move(rhs.tasks);
  }

  priority_taskq& operator=(priority_taskq&& rhs) {
    if (this != &rhs) {
      std::unique_lock lk(mu, std::defer_lock), rhs_lk(rhs.mu, std::defer_lock);
      std::lock(lk, rhs_lk);
      tasks = std::move(rhs.tasks);
    }
    return *this;
  }

  std::size_t put(PriorityTaskType& p) {
    std::unique_lock l(mu);
    append(std::move(p));
    std::ranges::push_heap(tasks, [](auto&& a, auto&& b) {
      return a <=> b < 0;
    });
    return 1;
  }

  std::size_t put(PriorityTaskType&& p) {
    std::unique_lock l(mu);
    append(std::move(p));
    std::push_heap(tasks.begin(), tasks.end(), [](auto&& a, auto&& b) {
      return a <=> b < 0;
    });
    return 1;
  }

  bool pop(std::unique_ptr<executable>& t) override {
    std::unique_lock l(mu);
    std::pop_heap(tasks.begin(), tasks.end(), [](auto&& a, auto&& b) {
      return a <=> b < 0;
    });
    return pop_back(t);
  }

  template<typename C, typename = std::enable_if_t<
    traits::is_container<C>::value && std::is_same_v<traits::FindTaskType<C>::type, PriorityTaskType>>
  >
  std::size_t put(C&& c) {
    std::unique_lock l(mu);
    auto n = c.size();
    tasks.insert(tasks.end(), std::make_move_iterator(c.begin()), std::make_move_iterator(c.end()));
    std::push_heap(tasks.begin(), tasks.end(), [](auto&& a, auto&& b) {
        return a <=> b < 0;
      });
    return n;
  }

  std::size_t len() const override {
    std::shared_lock l(mu);
    return tasks.size();
  }

  virtual ~priority_taskq() = default;

protected:
  template<typename InputIter>
  task_queue& insert(InputIter s, InputIter e) { // check
    std::unique_lock l(mu);
    tasks.insert(tasks.end(), s, e);
    std::push_heap(tasks.begin(), tasks.end(), [](auto&& a, auto&& b) {
        return a <=> b < 0;
    });
    return *this;
  }

  void append(PriorityTaskType&& p) {
    tasks.emplace_back(std::move(p));
  }

  bool pop_back(std::unique_ptr<executable>& t) {
    bool ret = false;
    if (!tasks.empty()) {
      t = std::make_unique<PriorityTaskType>(std::move(tasks.back()));
      tasks.pop_back();
      ret = true;
    }
    return ret;
  }

  mutable std::shared_mutex mu; // all mutex type are neither copyable nor movable
  std::deque<PriorityTaskType> tasks;
};

template<typename Ret>
class priority_taskq<Ret, void> : public task_queue {
public:
  using PriorityTaskType = priority_task<Ret, void>;

  explicit priority_taskq()
  : mu{}
  , tasks{}
  {}

  priority_taskq(priority_taskq&& rhs)
  : mu{}
  , tasks{}
  {
    std::lock_guard lk(rhs.mu);
    tasks = std::move(rhs.tasks);
  }

  priority_taskq& operator=(priority_taskq&& rhs) {
    if (this != &rhs) {
      std::unique_lock lk(mu, std::defer_lock), rhs_lk(rhs.mu, std::defer_lock);
      std::lock(lk, rhs_lk);
      tasks = std::move(rhs.tasks);
    }
    return *this;
  }

  std::size_t put(PriorityTaskType& p) {
    std::unique_lock l(mu);
    tasks.emplace_back(std::move(p));
    return 1;
  }

  void put(PriorityTaskType&& p) {
    std::unique_lock l(mu);
    tasks.emplace_back(std::move(p));
  }

  bool pop(std::unique_ptr<executable>& t) override {
    std::unique_lock l(mu);
    return pop_back(t);
  }

  template<typename C, typename = std::enable_if_t<
    traits::is_container<C>::value && std::is_same_v<traits::FindTaskType<C>::type, PriorityTaskType>>
  >
  std::size_t put(C&& c) {
    std::unique_lock l(mu);
    std::size_t n = c.size();
    tasks.insert(tasks.end(), std::make_move_iterator(c.begin()), std::make_move_iterator(c.end()));
    return n;
  }

  std::size_t len() const override {
    std::shared_lock l(mu);
    return tasks.size();
  }

  virtual ~priority_taskq() = default;

protected:
  bool pop_back(std::unique_ptr<executable>& t) {
    bool ret = false;
    if (!tasks.empty()) {
      t = std::make_unique<PriorityTaskType>(std::move(tasks.back()));
      tasks.pop_back();
      ret = true;
    }
    return ret;
  }

  mutable std::shared_mutex mu; // all mutex type are neither copyable nor movable
  std::deque<PriorityTaskType> tasks;
};

} // namespace details

template<typename T>
struct priority_taskq : details::priority_taskq<typename T::ReturnType, typename T::PriorityType> {};

} // namespace thp

#endif // TASK_QUEUE_HPP_
