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

template<typename T>
struct regular_task;

template <typename Ret, typename... Args>
struct regular_task<Ret(Args...)> : public executable {

  template <typename Fn>
  constexpr regular_task(Fn&& fn)
    : pt{std::forward<Fn>(fn)}
  {}

  regular_task(regular_task&&) = default;
  regular_task& operator = (regular_task&&) = default;

private:
  std::packaged_task<Ret(Args...)> pt;

};

template<typename Prio = void>
//requires std::totally_ordered<Prio>
struct comparable_task : virtual public executable {
  using PriorityType = Prio;

  constexpr comparable_task() = default;

  constexpr comparable_task(Prio&& p)
  : priority{std::forward<Prio>(p)}
  {}

  constexpr decltype(auto) set_priority(Prio&& prio) {
    priority = std::forward<Prio>(prio);
    return *this;
  }

  template<typename T>
  //requires std::totally_ordered_with<Prio, typename T::PriorityType>
  constexpr auto operator <=> (const T& rhs) const
  {
    return this->priority <=> rhs.priority;
  }

protected:
  Prio priority;
};

template<>
struct comparable_task<void> : virtual public executable {
  using PriorityType = void;

  template<typename T>
  constexpr auto operator <=> (const T& ) const
  {
    return std::strong_ordering::less;
  }
};

template <typename Ret, typename Prio = void>
class priority_task : public virtual comparable_task<Prio> {
public:
  using ReturnType = Ret;
  using PriorityType = Prio;

  template <typename Fn, typename... Args>
  requires std::regular_invocable<Fn,Args...>
           && std::same_as<Ret, std::invoke_result_t<Fn,Args...>>
  constexpr explicit priority_task(Fn &&fn, Args &&... args)
    : comparable_task<Prio>()
    , pt{std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...)}
  {}

  priority_task(priority_task&&) = default;
  priority_task& operator = (priority_task&&) = default;

  constexpr std::ostream& info(std::ostream& oss) override {
    return oss;
  }

  std::future<Ret> future() { return pt.get_future(); }

  void execute() override { pt(); }

  virtual ~priority_task() = default;

protected:
  std::packaged_task<Ret()> pt;
};

template<typename T>
using simple_task  = priority_task<T, void>;

} // namespace thp

#endif // TASK_TYPE_HPP_
