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

namespace thp {

// simple task, could be shared across threads
template<typename Ret>
struct simple_task : virtual public executable {
  using ReturnType = Ret;
  using PriorityType = void;

  template <typename Fn, typename... Args>
  requires std::regular_invocable<Fn,Args...>
           && std::same_as<Ret, std::invoke_result_t<Fn,Args...>>
  constexpr explicit simple_task(Fn &&fn, Args &&... args)
      : pt{std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...)}
  {}

  simple_task(simple_task&&) = default;
  simple_task& operator = (simple_task&&) = default;

  void execute() override { pt(); }

  std::future<Ret> future() { return pt.get_future(); }

  virtual std::ostream& info(std::ostream& oss) override {
    oss << typeid(decltype(pt)).name();
    return oss;
  }

  virtual ~simple_task() = default;

protected:
  std::packaged_task<Ret()> pt;
};

template <typename Ret, typename Prio>
requires std::totally_ordered<Prio>
class priority_task : public simple_task<Ret> {
public:
  using ReturnType = Ret;
  using PriorityType = Prio;

  template <typename Fn, typename... Args>
  requires std::regular_invocable<Fn,Args...>
           && std::same_as<Ret, std::invoke_result_t<Fn,Args...>>
  constexpr explicit priority_task(Fn &&fn, Args &&... args)
      : simple_task<Ret>{std::forward<Fn>(fn), std::forward<Args>(args)...}
      , priority {}
  {}

  priority_task(priority_task&&) = default;
  priority_task& operator = (priority_task&&) = default;

  constexpr decltype(auto) set_priority(Prio&& prio) {
    priority = std::forward<Prio>(prio);
    return std::ref(*this);
  }

  constexpr auto get_priority() const { return priority; }

  template<kncpt::PriorityTask T>
  constexpr std::strong_ordering operator <=> (const T& rhs) const
  {
    return this->get_priority() <=> rhs.get_priority();
  }

  constexpr std::ostream& info(std::ostream& oss) override {
    simple_task<Ret>::info(oss);
    //oss << priority;
    return oss;
  }

  virtual ~priority_task() = default;

protected:
//  simple_task<Ret> tak;
  Prio priority;
};

template<typename Fn, typename... Args>
[[nodiscard]] constexpr inline decltype(auto) make_task(Fn&& fn, Args&& ...args) {
  using Ret = std::invoke_result_t<Fn,Args...>;
  return std::make_unique<simple_task<Ret>>(std::forward<Fn>(fn), std::forward<Args>(args)...);
}

template<typename Prio, typename Fn, typename... Args>
[[nodiscard]] constexpr inline decltype(auto) make_task(Fn&& fn, Args&& ...args) {
   using Ret = std::invoke_result_t<Fn,Args...>;
   return std::make_unique<priority_task<Ret, Prio>>(std::forward<Fn>(fn), std::forward<Args>(args)...);
}

} // namespace thp

#endif // TASK_TYPE_HPP_
