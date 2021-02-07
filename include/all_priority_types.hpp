#ifndef ALL_PRIORITY_TYPES_HPP__
#define ALL_PRIORITY_TYPES_HPP__

#include <chrono>
#include <tuple>
#include <vector>

#include "include/task_type.hpp"

namespace thp {

using AllPriorityTaskTupleType = std::tuple<
simple_task<int>,
priority_task<int, void>,
priority_task<void, void>,
priority_task<unsigned, void>,
priority_task<unsigned, int>,
priority_task<unsigned, float>,
priority_task<std::vector<unsigned int>, void>,
priority_task<unsigned, std::chrono::steady_clock::time_point>,
priority_task<unsigned, std::chrono::system_clock::time_point>,
priority_task<unsigned, std::chrono::high_resolution_clock::time_point>
>;

} // namespace thp

#endif // ALL_PRIORITY_TYPES_HPP__
