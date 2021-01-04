#ifndef TIME_TASK_TYPE_HPP_
#define TIME_TASK_TYPE_HPP_

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <type_traits>
#include <ranges>
#include <algorithm>
#include <initializer_list>

#include "include/task_type.hpp"
#include "include/traits.hpp"

namespace thp {

template <typename Ret, typename Clock = std::chrono::system_clock>
class time_task : public priority_task<Ret, typename Clock::time_point> {
public:
  using ReturnType = Ret;
  using PriorityType = typename Clock::time_point;
  using TimePoint = PriorityType;
  using Base = priority_task<Ret, typename Clock::time_point>;

  template <typename Fn, typename... Args>
  requires std::regular_invocable<Fn,Args...>
           && std::same_as<Ret, std::invoke_result_t<Fn,Args...>>
  explicit time_task(Fn &&fn, Args &&... args)
      : Base{std::forward<Fn>(fn), std::forward<Args>(args)...}
  {
    Base::set_priority(Clock::now());
  }

  decltype(auto) at(TimePoint&& tp) {
    Base::set_priority(std::forward<TimePoint>(tp));
    return *this;
  }

  void execute() override {
    // std::cerr << std::this_thread::get_id() << " : " <<
    // (at-Clock::now()).count() << ", will sleep till " << std::endl;
    std::this_thread::sleep_until(Base::get_priority());
    // std::cerr << std::this_thread::get_id() << " : " <<
    // (at-Clock::now()).count() << ", wakeup, execute " << std::endl;
    Base::execute();
  }

  virtual ~time_task() = default;
};

template <typename Ret, typename Clock = std::chrono::system_clock>
class time_series_task : public time_task<Ret, Clock> {
public:
  using ReturnType = Ret;
  using PriorityType = typename Clock::time_point;
  using TimePoint = PriorityType;
  using Base = time_task<Ret, Clock>;

  template <typename Fn, typename... Args>
  requires std::regular_invocable<Fn,Args...>
           && std::same_as<Ret, std::invoke_result_t<Fn,Args...>>
  explicit time_series_task(Fn &&fn, Args &&... args)
      : Base{std::forward<Fn>(fn), std::forward<Args>(args)...}
      , time_points{}
  {}

  decltype(auto) at(std::initializer_list<TimePoint> at) {
    time_points = std::vector<TimePoint>{at};
    std::ranges::sort(time_points);
    return *this;
  }

  void execute() override {
    std::ranges::for_each(time_points, [this](auto&& tp) {
       //std::cerr << std::this_thread::get_id() << " : " <<
       //(at-Clock::now()).count() << ", will sleep till " << std::endl;
      std::this_thread::sleep_until(tp);
       //std::cerr << std::this_thread::get_id() << " : " <<
       //(at-Clock::now()).count() << ", wakeup, execute " << std::endl;
      Base::pt();
      Base::pt.reset();
    });
  }

  virtual ~time_series_task() = default;

protected:
  std::vector<TimePoint> time_points;
};

} // namespace thp

#endif // TIME_TASK_TYPE_HPP_
