#ifndef TIME_TASK_TYPE_HPP_
#define TIME_TASK_TYPE_HPP_

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <type_traits>

#include "include/priority_task.hpp"
#include "include/task_type.hpp"
#include "include/traits.hpp"

namespace thp {

template <typename Ret, typename Clock = std::chrono::system_clock>
class time_task : public task {
public:
  using ReturnType = Ret;
  using PriorityType = typename Clock::time_point;

  template <typename Fn, typename... Args,
            typename = std::enable_if_t<
                std::is_same_v<Ret, std::invoke_result_t<Fn, Args...>>>>
  explicit time_task(Fn &&fn, Args &&... args)
      : work{std::forward<Fn>(fn), std::forward<Args>(args)...}, at{} {}

  explicit time_task(simple_task<Ret> &&tsk, PriorityType &&p)
      : work{std::move(tsk)} {
    at = std::forward<PriorityType>(p);
  }

  void execute() override {
    // std::cerr << std::this_thread::get_id() << " : " <<
    // (at-Clock::now()).count() << ", will sleep till " << std::endl;
    std::this_thread::sleep_until(at);
    // std::cerr << std::this_thread::get_id() << " : " <<
    // (at-Clock::now()).count() << ", wakeup, execute " << std::endl;
    work.execute();
  }

  virtual ~time_task() = default;

protected:
  simple_task<Ret> work;
  typename Clock::time_point at;
};

} // namespace thp

#endif // TIME_TASK_TYPE_HPP_
