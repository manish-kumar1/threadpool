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
  virtual std::size_t pop(std::shared_ptr<executable>&) = 0;
  virtual std::size_t pop_n(std::deque<std::shared_ptr<executable>>& out, std::size_t n = 1) = 0;
  virtual std::size_t len() const = 0;
  virtual bool empty() const { return 0u == len(); }
  virtual ~task_queue() = default;
};

template <typename C>
constexpr decltype(auto) to_value_type(C&& t)
{
  using T = std::remove_cvref_t<decltype(t)>;

  if constexpr (traits::is_unique_ptr<T>::value)                 { return std::make_shared<T>(std::move(t)); }
  else if constexpr (traits::is_shared_ptr<T>::value)            { return std::move(t); }
  else if constexpr (traits::is_reference_wrapper<T>::value)     { return std::move(t.get()); }
  else if constexpr (std::is_base_of_v<executable, T>)           { return std::make_shared<T>(std::move(t)); }
  else if constexpr (traits::is_container<T>::value) {
    using TaskType = traits::FindTaskType<std::remove_cvref_t<typename T::value_type>>::type;
    std::vector<std::shared_ptr<TaskType>> tasks;
    tasks.reserve(t.size());
    std::ranges::transform(t, std::back_inserter(tasks),
                           [&](auto&& x) { return to_value_type(x); });
    return tasks;
  }
  else {
    static_assert("type is not supported");
  }
}

//
// one executable container queue
// class with mutex, implement custom move constructors, as they are defaulted to delete
//
template<typename Prio, typename TaskComp = std::less<comparable_task<Prio>>>
class priority_taskq : public task_queue {
  using value_type = std::shared_ptr<comparable_task<Prio>>;

  struct value_compare {
	  constexpr decltype(auto) operator () (auto&& x, auto&& y) const
	  {
		  return TaskComp()(*x, *y);
	  };
  };

public:
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

  std::size_t pop(std::shared_ptr<executable>& t) override {
    std::unique_lock l(mu);
    if (tasks.empty()) return 0;

    if constexpr (std::is_same_v<void, Prio>) {
    	t = std::move(tasks.front());
      //std::cerr << "pop " << t << ", " << t.get() << std::endl;
    	tasks.pop_front();
    }
    else {
		  std::ranges::pop_heap(tasks, value_compare());
		  t = std::move(tasks.back());
      //std::cerr << "pop " << t << ", " << t.get() << std::endl;
		  tasks.pop_back();
    }
	  return 1;
  }

  std::size_t pop_n(std::deque<std::shared_ptr<executable>>& out, std::size_t n = 1u) {
    std::unique_lock l(mu);
    if (tasks.empty()) return 0;

    n = std::min(n, tasks.size());

    for(auto x = n; x > 0; --x) {
      if constexpr (std::is_same_v<void, Prio>) {
    	  out.emplace_back(std::move(tasks.front()));
    	  tasks.pop_front();
      }
      else {
		    std::ranges::pop_heap(tasks, value_compare());
		    out.emplace_back(std::move(tasks.back()));
		    tasks.pop_back();
      }
    }
    return n;
  }

  template<typename C>
  std::size_t put(C&& c) {
    std::unique_lock l(mu);
    return insert(to_value_type(std::forward<C>(c)));
  }

  std::size_t len() const override {
    std::shared_lock l(mu);
    return tasks.size();
  }

  virtual ~priority_taskq() = default;

protected:
  std::size_t insert(std::shared_ptr<comparable_task<Prio>>&& p) {
    tasks.emplace_back(std::move(p));
    std::ranges::push_heap(tasks, value_compare());
    return 1;
  }

  template<typename T>
  std::size_t insert(std::vector<std::shared_ptr<T>>&& c) {
    auto n = c.size();
    tasks.insert(tasks.end(), std::make_move_iterator(c.begin()), std::make_move_iterator(c.end()));
    std::ranges::push_heap(tasks, value_compare());
    return n;
  }

  mutable std::shared_mutex mu; // all mutex type are neither copyable nor movable
  std::deque<value_type> tasks;
};

} // namespace thp

#endif // TASK_QUEUE_HPP_
