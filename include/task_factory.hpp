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
  static_assert(compile_time::find<priority_task<Ret, void>, AllPriorityTaskTupleType>() < std::tuple_size_v<AllPriorityTaskTupleType>);
  //static_assert(compile_time::exists_in<priority_task<Ret, void>, AllPriorityTaskTupleType>(), "task type is not registered");
  return priority_task<Ret, void>(std::forward<Fn>(fn), std::forward<Args>(args)...);
}

template<typename Prio, typename Fn, typename... Args>
[[nodiscard]] constexpr inline decltype(auto) make_task(Fn&& fn, Args&& ...args) {
  using Ret = std::invoke_result_t<Fn,Args...>;
  static_assert(compile_time::find<priority_task<Ret, Prio>, AllPriorityTaskTupleType>() < std::tuple_size_v<AllPriorityTaskTupleType>);
  //static_assert(compile_time::exists_in<priority_task<Ret, Prio>, AllPriorityTaskTupleType>(), "task type is not registered");
  return priority_task<Ret, Prio>(std::forward<Fn>(fn), std::forward<Args>(args)...);
}

}

#endif // TASK_FACTORY_HPP__
