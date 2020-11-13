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

// simple task, wrapper around packaged_task
template <typename Ret, typename Prio = void>
class simple_task : public task {
public:
  using ReturnType = Ret;
  using PriorityType = Prio;

  template <typename Fn, typename... Args,
            typename = std::enable_if_t<
                std::is_same_v<Ret, std::invoke_result_t<Fn, Args...>>>>
  explicit simple_task(Fn &&fn, Args &&... args)
      : pt{std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...)}
      , priority {}
  {}

  simple_task(simple_task &&) = default;
  simple_task &operator=(simple_task &&) = default;

  simple_task(const simple_task &) = delete;
  simple_task &operator=(const simple_task &) = delete;

  void execute() override { pt(); }

  std::future<Ret> future() { return pt.get_future(); }

  decltype(auto) set_priority(Prio&& prio) {
    priority = std::forward<Prio>(prio);
    return std::ref(*this);
  }

  template<typename T>
  auto operator <=> (T&& rhs) {
    static_assert(std::is_same_v<typename T::PriorityType, PriorityType>);
    return this->priority <=> rhs.priority;
  }

  std::ostream& operator << (std::ostream& oss) {
    oss <<  typeid(PriorityType).name() << ',' << priority;
    return oss;
  }

  virtual ~simple_task() = default;

protected:
  std::packaged_task<Ret()> pt;
  Prio priority;
};

// simple task, could be shared across threads
template <typename Ret>
class simple_task<Ret, void> : public task {
public:
  using ReturnType = Ret;
  using PriorityType = void;

  template <typename Fn, typename... Args,
            typename = std::enable_if_t<
                std::is_same_v<Ret, std::invoke_result_t<Fn, Args...>>>>
  explicit simple_task(Fn &&fn, Args &&... args)
      : pt{std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...)}
  {}

  simple_task(simple_task &&) = default;
  simple_task &operator=(simple_task &&) = default;

  simple_task(const simple_task &) = delete;
  simple_task &operator=(const simple_task &) = delete;

  void execute() override {
    pt();
  }

  std::future<Ret> future() { return pt.get_future(); }

  std::ostream& operator << (std::ostream& oss) {
    oss << typeid(PriorityType).name() << ",void";
    return oss;
  }

  virtual ~simple_task() = default;

protected:
  std::packaged_task<Ret()> pt;
};

} // namespace thp

#endif // TASK_TYPE_HPP_
