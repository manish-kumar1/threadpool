#ifndef TP_CONCEPTS_HPP_

#include <concepts>

#include "include/executable.hpp"
#include "include/traits.hpp"
#include "include/task_type.hpp"

namespace thp {
namespace kncpt {

template <typename T>
concept SimpleTask = std::is_base_of_v<executable, T> &&
  requires {
    typename T::ReturnType;
  };

template <typename T>
concept PriorityTask = SimpleTask<T> &&
  requires {
    typename T::PriorityType;
    requires std::totally_ordered<typename T::PriorityType>;
  };

} // namespace kncpt
} // namespace thp


#endif // TP_CONCEPTS_HPP_
