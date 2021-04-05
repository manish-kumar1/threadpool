#ifndef TP_CONCEPTS_HPP_

#include <concepts>
#include <chrono>

#include "include/executable.hpp"
#include "include/traits.hpp"

namespace thp {
namespace kncpt {

template <typename T>
concept SimpleTask = std::is_base_of_v<executable, T> &&
  requires {
    typename T::ReturnType;
    std::same_as<typename T::PriorityType, void>;
  };

template <typename T>
concept PriorityTask = SimpleTask<T> &&
  requires {
    typename T::PriorityType;
    requires std::totally_ordered<typename T::PriorityType>;
  };

template<typename T>
concept ThreadPoolTask = PriorityTask<typename traits::FindTaskType<T>::type> || SimpleTask<typename traits::FindTaskType<T>::type>;

} // namespace kncpt
} // namespace thp


#endif // TP_CONCEPTS_HPP_
