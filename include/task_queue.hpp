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
  virtual std::size_t pop(std::unique_ptr<executable>&) = 0;
  virtual std::size_t pop_n(std::deque<std::unique_ptr<executable>>& out, std::size_t n = 1) = 0;
  virtual std::size_t len() const = 0;
  virtual bool empty() { return 0u == len(); }
  virtual ~task_queue() = default;
};

namespace details {
// one executable container queue
// class with mutex, implement custom move constructors, as
// they are defaulted to delete
template<typename Ret, typename Prio = void, typename Comp = std::less<priority_task<Ret, Prio>>>
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
    std::ranges::push_heap(tasks, Comp());
    return 1;
  }

  std::size_t put(PriorityTaskType&& p) {
    std::unique_lock l(mu);
    append(std::move(p));
    std::ranges::push_heap(tasks, Comp());
    return 1;
  }

  std::size_t pop(std::unique_ptr<executable>& t) override {
    std::unique_lock l(mu);
    if (tasks.empty()) return 0;
    std::ranges::pop_heap(tasks, Comp());
    t = std::make_unique<PriorityTaskType>(std::move(tasks.back()));
    tasks.pop_back();
    return 1;
  }

  std::size_t pop_n(std::deque<std::unique_ptr<executable>>& out, std::size_t n = 1u) {
    std::unique_lock l(mu);
    if (tasks.empty()) return false;

    n = std::min(n, tasks.size());

    for(auto x = n; x > 0; --x) {
      std::ranges::pop_heap(tasks, Comp());
      out.emplace_back(std::make_unique<PriorityTaskType>(std::move(tasks.back())));
      tasks.pop_back();
    }
    return n;
  }

  template<typename C, typename = std::enable_if_t<
    traits::is_container<C>::value && std::is_same_v<traits::FindTaskType<C>::type, PriorityTaskType>>
  >
  std::size_t put(C&& c) {
    std::unique_lock l(mu);
    auto n = c.size();
    tasks.insert(tasks.end(), std::make_move_iterator(c.begin()), std::make_move_iterator(c.end()));
    std::ranges::push_heap(tasks, Comp());
    return n;
  }

  std::size_t len() const override {
    std::shared_lock l(mu);
    return tasks.size();
  }

  virtual ~priority_taskq() = default;

protected:
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

  std::size_t put(PriorityTaskType&& p) {
    std::unique_lock l(mu);
    tasks.emplace_back(std::move(p));
    return 1;
  }

  std::size_t pop(std::unique_ptr<executable>& t) override {
    std::unique_lock l(mu);
    if (tasks.empty()) return 0;
    t = std::make_unique<PriorityTaskType>(std::move(tasks.back()));
    tasks.pop_back();
    return 1;
  }

  std::size_t pop_n(std::deque<std::unique_ptr<executable>>& out, std::size_t n = 1u) {
    std::unique_lock l(mu);
    if (tasks.empty()) return false;

    n = std::min(n, tasks.size());

    for(auto x = n; x > 0; --x) {
      out.emplace_back(std::make_unique<PriorityTaskType>(std::move(tasks.back())));
      tasks.pop_back();
    }
    return n;
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
  mutable std::shared_mutex mu; // all mutex type are neither copyable nor movable
  std::deque<PriorityTaskType> tasks;
};

} // namespace details

template<typename T>
struct priority_taskq : details::priority_taskq<typename T::ReturnType, typename T::PriorityType> {};

} // namespace thp

#endif // TASK_QUEUE_HPP_
