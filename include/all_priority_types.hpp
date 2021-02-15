#ifndef ALL_PRIORITY_TYPES_HPP__
#define ALL_PRIORITY_TYPES_HPP__

#include <chrono>
#include <tuple>

#include "include/task_type.hpp"

namespace thp {

using AllPriorityTupleType = std::tuple<
void,
int,
float,
std::chrono::steady_clock::time_point,
std::chrono::system_clock::time_point
>;

} // namespace thp

#endif // ALL_PRIORITY_TYPES_HPP__
