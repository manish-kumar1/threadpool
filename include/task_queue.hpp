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
  virtual bool empty() const { return 0u == len(); }
  virtual ~task_queue() = default;
};

//
// one executable container queue
//
template<typename Prio, typename TaskComp = std::less<comparable_task<Prio>>>
class priority_taskq : public task_queue {
  using value_type = std::unique_ptr<comparable_task<Prio>>;

  struct value_compare {
	  constexpr decltype(auto) operator () (auto&& x, auto&& y) const
	  {
		  return TaskComp()(*x, *y);
	  };
  };

  template <typename C>
  constexpr decltype(auto) to_value_type(C&& t)
  {
    using T = std::remove_cvref_t<decltype(t)>;

    if constexpr (traits::is_unique_ptr<T>::value)      { return std::move(t); }
    else if constexpr (std::is_base_of_v<executable, T>){ return std::make_unique<T>(std::move(t)); }
    else if constexpr (traits::is_container<T>::value)  { return std::move(t); }
    else {
      static_assert("type is not supported");
    }
  }

public:
  constexpr explicit priority_taskq()
  : mu{}
  , tasks{}
  {}

  priority_taskq(const priority_taskq&) = delete;
  priority_taskq& operator = (const priority_taskq&) = delete;

  priority_taskq(priority_taskq&& rhs) noexcept
  : mu{}
  , tasks{}
  {
    std::lock_guard lk(rhs.mu);
    std::swap(tasks, rhs.tasks);
  }

  priority_taskq& operator=(priority_taskq&& rhs) noexcept {
    std::unique_lock lk(mu, std::defer_lock), rhs_lk(rhs.mu, std::defer_lock);
    std::lock(lk, rhs_lk);
    if (this != &rhs) {
      tasks = std::move(rhs.tasks);
    }
    return *this;
  }

  constexpr std::size_t pop(std::unique_ptr<executable>& t) override {
    std::unique_lock l(mu);
    if (tasks.empty()) return 0;

    if constexpr (std::is_same_v<void, Prio>) {
    	t = std::move(tasks.front());
    	tasks.pop_front();
    }
    else {
		  std::ranges::pop_heap(tasks, value_compare());
		  t = std::move(tasks.back());
		  tasks.pop_back();
    }
	  return 1;
  }

  constexpr std::size_t pop_n(std::deque<std::unique_ptr<executable>>& out, std::size_t n) {
    std::unique_lock l(mu);
    if (tasks.empty()) return 0;

    n = std::min(n, tasks.size());

    if constexpr (std::is_same_v<void, Prio>) {
      std::move(tasks.begin(), std::next(tasks.begin(), n), std::back_inserter(out));
      tasks.erase(tasks.begin(), std::next(tasks.begin(), n));
    } else {
      std::ranges::sort_heap(tasks, value_compare());
      std::move(tasks.begin(), std::next(tasks.begin(), n), std::back_inserter(out));
      tasks.erase(tasks.begin(), std::next(tasks.begin(), n));
      std::ranges::make_heap(tasks, value_compare());
    }
    return 0;
  }

  template<typename... C>
  constexpr std::size_t put(C&&... c) {
    std::unique_lock l(mu);
    std::size_t ret = 0;
    ((ret += _insert(to_value_type(std::forward<C>(c)))), ...);
    return ret;
  }

  constexpr std::size_t len() const override {
    std::unique_lock l(mu);
    return tasks.size();
  }

  virtual ~priority_taskq() noexcept {
    std::unique_lock l(mu);
    tasks.clear();
  }

protected:
  constexpr std::size_t _insert(value_type&& p) {
    tasks.emplace_back(std::move(p));
    if constexpr (!std::is_same_v<void, Prio>) {
      std::ranges::push_heap(tasks, value_compare());
    }
    return 1;
  }

  template<typename T>
  constexpr std::size_t _insert(std::vector<T>&& c) {
    auto n = c.size();
    std::ranges::for_each(c, [&](auto&& t) {
      _insert(to_value_type(std::move(t)));
    });

    if constexpr (!std::is_same_v<void, Prio>) {
      std::ranges::make_heap(tasks, value_compare());
    }
    return n;
  }

  mutable std::mutex mu;
  std::deque<value_type> tasks;
};

} // namespace thp

#endif // TASK_QUEUE_HPP_
