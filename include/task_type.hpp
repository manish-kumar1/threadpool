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

template <typename Ret>
class regular_task: public virtual executable {
public:
  template <typename Fn, typename...Args>
  requires std::regular_invocable<Fn,Args...>
           && std::same_as<Ret, std::invoke_result_t<Fn,Args...>>
  constexpr explicit regular_task(Fn&& fn, Args&&... args)
    : pt{std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...)}
  {}

  regular_task(regular_task&&) = default;
  regular_task& operator = (regular_task&&) = default;

  void execute() override {
    pt();
  }

  std::future<Ret> future() { return pt.get_future(); }

  virtual ~regular_task() noexcept = default;

protected:
  std::packaged_task<Ret()> pt;
};

template<typename Prio = void>
//requires std::totally_ordered<Prio>
struct comparable_task : virtual public executable {
  using PriorityType = Prio;

  constexpr explicit comparable_task() = default;

  comparable_task(comparable_task&&) = default;
  comparable_task& operator = (comparable_task&&) = default;

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

  virtual ~comparable_task() noexcept = default;
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

template <typename Ret, typename Prio>
class priority_task : public regular_task<Ret>, public comparable_task<Prio> {
public:
  using ReturnType = Ret;
  using PriorityType = Prio;

  template <typename Fn, typename... Args>
  constexpr explicit priority_task(Fn &&fn, Args &&... args)
    : regular_task<Ret>{std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...)}
    , comparable_task<Prio>{}
  {}

  priority_task(priority_task&&) = default;
  priority_task& operator = (priority_task&&) = default;

  constexpr std::ostream& info(std::ostream& oss) override {
    return oss;
  }

  virtual ~priority_task() noexcept = default;
};

template<typename T>
using simple_task  = priority_task<T, void>;

} // namespace thp

#endif // TASK_TYPE_HPP_
