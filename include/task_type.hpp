#ifndef TASK_TYPE_HPP_
#define TASK_TYPE_HPP_

#include <concepts>
#include <functional>
#include <future>
#include <memory>
#include <type_traits>
#include <compare>

#include "include/executable.hpp"
#include "include/concepts.hpp"
#include "include/register_types.hpp"

namespace thp {

//template<typename T>
//struct regular_task;
//
//template <typename Ret, typename... Args>
//struct regular_task<Ret(Args...)> : public executable {
//
//  template <typename Fn>
//  constexpr regular_task(Fn&& fn)
//    : pt{std::forward<Fn>(fn)}
//  {}
//
//  regular_task(regular_task&&) = default;
//  regular_task& operator = (regular_task&&) = default;
//
//private:
//  std::packaged_task<Ret(Args...)> pt;
//
//};
//
//template<typename Prio = void>
////requires std::totally_ordered<Prio>
//struct comparable_task : virtual public executable {
//  using PriorityType = Prio;
//
//  constexpr comparable_task(Prio&& p)
//  : priority{std::forward<Prio>(p)}
//  {}
//
//  constexpr decltype(auto) set_priority(Prio&& prio) {
//    priority = std::forward<Prio>(prio);
//    return *this;
//  }
//
//  // //constexpr auto get_priority() const { return priority; }
//
//  // constexpr bool operator < (const comparable_task& rhs) const { return this->priority < rhs.priority; }
//
//  template<kncpt::PriorityTask T>
//  //requires std::totally_ordered_with<Prio, typename T::PriorityType>
//  constexpr auto operator <=> (const T& rhs) const
//  {
//    return this->priority <=> rhs.priority;
//  }
//
//private:
//  Prio priority;
//};
//
//template<>
//struct comparable_task<void> : virtual public executable {
//  using PriorityType = void;
//
//  template<kncpt::PriorityTask T>
//  constexpr auto operator <=> (const T& rhs) const
//  {
//    return std::strong_ordering{};
//  }
//};

template <typename Ret, typename Prio = void>
class priority_task : public virtual executable {
public:
  using ReturnType = Ret;
  using PriorityType = Prio;

  template <typename Fn, typename... Args>
  requires std::regular_invocable<Fn,Args...>
           && std::same_as<Ret, std::invoke_result_t<Fn,Args...>>
  constexpr explicit priority_task(Fn &&fn, Args &&... args)
      : pt{std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...)}
      , priority{}
  {}

  priority_task(priority_task&&) = default;
  priority_task& operator = (priority_task&&) = default;

  constexpr std::ostream& info(std::ostream& oss) override {
    return oss;
  }

  std::future<Ret> future() { return pt.get_future(); }

  void execute() override { pt(); }

  constexpr decltype(auto) set_priority(Prio&& prio) {
    priority = std::forward<Prio>(prio);
    return *this;
  }

  template<kncpt::PriorityTask T>
  requires std::totally_ordered_with<Prio, typename T::PriorityType>
  constexpr auto operator <=> (const T& rhs) const
  {
    return this->priority <=> rhs.priority;
  }

  virtual ~priority_task() = default;

protected:
  std::packaged_task<Ret()> pt;
  Prio priority;
};

// simple task, could be shared across threads
template<typename Ret>
struct priority_task<Ret, void> : virtual public executable {
  using ReturnType = Ret;
  using PriorityType = void;

  template <typename Fn, typename... Args>
  requires std::regular_invocable<Fn,Args...>
           && std::same_as<Ret, std::invoke_result_t<Fn,Args...>>
  constexpr explicit priority_task(Fn &&fn, Args &&... args)
      : pt{std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...)}
  {}

  priority_task(priority_task&&) = default;
  priority_task& operator = (priority_task&&) = default;

  void execute() override { pt(); }

  std::future<Ret> future() { return pt.get_future(); }

  template<kncpt::PriorityTask T>
  //requires std::totally_ordered_with<PriorityType, typename T::PriorityType>
  constexpr auto operator <=> (const T& rhs) const
  {
    return std::strong_ordering{};
  }

  virtual std::ostream& info(std::ostream& oss) override {
    oss << typeid(decltype(pt)).name();
    return oss;
  }

  virtual ~priority_task() = default;

protected:
  std::packaged_task<Ret()> pt;
};

template<typename T>
struct simple_task : priority_task<T, void> {
  using ReturnType = priority_task<T, void>::ReturnType;
  using PriorityType = priority_task<T, void>::PriorityType;
};

} // namespace thp

#endif // TASK_TYPE_HPP_
