#ifndef TASK_TYPE_HPP_
#define TASK_TYPE_HPP_

#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <tuple>
#include <type_traits>

namespace thp {

// task interface
struct executable {
  virtual void execute() = 0;
  virtual ~executable() = default;
};

template<typename T = int>
class priority {
public:
  template<typename P>
  decltype(auto) operator <=> (const priority<P>& rhs) const {
    static_assert(std::is_convertible_v<T, P>);
    return val <=> T(rhs.val);
  }
  T val;
};

// simple task, could be shared across threads
template<typename Ret>
struct simple_task : virtual public executable {
  using ReturnType = Ret;
  using PriorityType = void;

  template <typename Fn, typename... Args,
            typename = std::enable_if_t<
                std::is_same_v<Ret, std::invoke_result_t<Fn, Args...>>>>
  explicit simple_task(Fn &&fn, Args &&... args)
      : pt{std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...)}
  {}

  simple_task(simple_task&&) = default;
  simple_task& operator = (simple_task&&) = default;

  void execute() override { pt(); }

  std::future<Ret> future() { return pt.get_future(); }
  virtual std::ostream& info(std::ostream& oss) {
    oss << typeid(decltype(pt)).name();
    return oss;
  }

  virtual ~simple_task() = default;

protected:
  std::packaged_task<Ret()> pt;
};

// simple task, wrapper around packaged_task
template <typename Ret, typename Prio>
class priority_task : public simple_task<Ret> {
public:
  using ReturnType = Ret;
  using PriorityType = Prio;

  template <typename Fn, typename... Args>
  explicit priority_task(Fn &&fn, Args &&... args)
      : simple_task<Ret>{std::forward<Fn>(fn), std::forward<Args>(args)...}
      , priority {}
  {}

  priority_task(priority_task&&) = default;
  priority_task& operator = (priority_task&&) = default;

  decltype(auto) set_priority(Prio&& prio) {
    priority = std::forward<Prio>(prio);
    return std::ref(*this);
  }

  template<typename T>
  auto operator <=> (T&& rhs) {
    static_assert(std::is_same_v<typename T::PriorityType, PriorityType>);
    return this->priority <=> rhs.priority;
  }
  std::ostream& info(std::ostream& oss) override {
    simple_task<Ret>::info(oss);
    //oss << priority;
    return oss;
  }

  virtual ~priority_task() = default;

protected:
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
