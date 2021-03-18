#ifndef ALL_PRIORITY_TYPES_HPP__
#define ALL_PRIORITY_TYPES_HPP__

#include <chrono>
#include <tuple>

#include "include/task_type.hpp"
#include "include/task_queue.hpp"

namespace thp {

using AllPriorityTupleType = std::tuple<void>;
using AllPriorityTupleType2 = std::tuple<
void,
int,
float,
std::chrono::steady_clock::time_point,
std::chrono::system_clock::time_point
>;

template<typename T>
struct TaskQueueTuple;

template<typename...Ts>
struct TaskQueueTuple<std::tuple<Ts...>> {
  using type = std::tuple<priority_taskq<Ts>...>;
};

} // namespace thp

#endif // ALL_PRIORITY_TYPES_HPP__
