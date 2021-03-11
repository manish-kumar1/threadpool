#ifndef TASK_FACTORY_HPP__
#define TASK_FACTORY_HPP__

#include <type_traits>
#include <functional>
#include <memory>

#include "include/task_type.hpp"
#include "include/all_priority_types.hpp"
#include "include/register_types.hpp"

namespace thp {

template<typename Fn, typename... Args>
[[nodiscard]] constexpr inline decltype(auto) make_task(Fn&& fn, Args&& ...args) {
  using Ret = std::invoke_result_t<Fn,Args...>;
  return std::make_shared<simple_task<Ret>>(std::forward<Fn>(fn), std::forward<Args>(args)...);
}

template<typename Prio, typename Fn, typename... Args>
[[nodiscard]] constexpr inline decltype(auto) make_task(Fn&& fn, Args&& ...args) {
  using Ret = std::invoke_result_t<Fn,Args...>;
  static_assert(compile_time::find<Prio, AllPriorityTupleType>() < std::tuple_size_v<AllPriorityTupleType>, "priority type not registered");
  return std::make_shared<priority_task<Ret, Prio>>(std::forward<Fn>(fn), std::forward<Args>(args)...);
}

}

#endif // TASK_FACTORY_HPP__
