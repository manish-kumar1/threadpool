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
#include "include/sync_container.hpp"

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
  constexpr explicit time_task(Fn &&fn, Args &&... args)
      : Base{std::forward<Fn>(fn), std::forward<Args>(args)...}
  {
    Base::set_priority(Clock::now());
  }

  constexpr decltype(auto) at(TimePoint&& tp) {
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
class time_series_task : public virtual executable {
  using ResultBuffer = sync_container<std::future<Ret>>;

public:
  using ReturnType = Ret;
  using PriorityType = typename Clock::time_point;
  using TimePoint = PriorityType;

  template <typename Fn, typename... Args>
  requires std::regular_invocable<Fn,Args...>
           && std::same_as<Ret, std::invoke_result_t<Fn,Args...>>
  explicit time_series_task(Fn &&fn, Args &&... args)
      : pt{std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...)}
      , time_points{}
      , results{new ResultBuffer(8)}
      , ret{}
  {
    ret.set_value(results);
  }

  time_series_task(time_series_task&&) = default;
  time_series_task& operator = (time_series_task&&) = default;

  decltype(auto) at(std::initializer_list<TimePoint> at) {
    time_points = std::vector<TimePoint>{at};
    std::ranges::sort(time_points);
    results->reserve(std::ranges::size(time_points));
    return *this;
  }

  decltype(auto) future() {
    return ret.get_future();
  }

  void execute() override {
    std::ranges::for_each(time_points, [&](auto&& tp) mutable {
      //std::cerr << std::this_thread::get_id() << " : " <<
      // (tp-Clock::now()).count() << ", will sleep till " << std::endl;
      this->results->put(this->pt.get_future());

      std::this_thread::sleep_until(tp);
      std::cerr << std::this_thread::get_id() << " : " << (tp-Clock::now()).count() << ", wakeup, execute " << std::endl;

      this->pt();
      this->pt.reset();
    });
    std::cerr << "time task done" << std::endl;
  }

  virtual ~time_series_task() = default;

protected:
  std::packaged_task<Ret()> pt;
  std::vector<TimePoint> time_points;
  std::shared_ptr<ResultBuffer> results;
  std::promise<std::shared_ptr<ResultBuffer>> ret;
//  std::shared_ptr<std::vector<std::future<Ret>> results;
};

} // namespace thp

#endif // TIME_TASK_TYPE_HPP_
