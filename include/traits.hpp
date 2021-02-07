#ifndef TRAITS_HPP
#define TRAITS_HPP

#include <array>
#include <chrono>
#include <deque>
#include <forward_list>
#include <list>
#include <memory>
#include <type_traits>
#include <vector>
#include <functional>
#include <tuple>

#include "include/task_type.hpp"

namespace thp {
namespace traits {

template <typename T, template <typename...> class Template>
struct is_specialization : std::false_type {};

template <template <typename...> class Template, typename... Args>
struct is_specialization<Template<Args...>, Template> : std::true_type {};

template <typename T> struct is_unique_ptr : std::false_type {};

template <typename T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};

template <typename T>
inline constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

template <typename T> struct is_shared_ptr : std::false_type {};

template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

template <typename T>
inline constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

template <typename T>
struct is_smart_ptr
    : std::integral_constant<bool, std::disjunction_v<is_unique_ptr<T>, is_shared_ptr<T>>> {};

template <typename T> struct is_timepoint : std::false_type {};

template <typename T>
struct is_timepoint<std::chrono::time_point<T>> : std::true_type {};

template<typename T>
struct is_reference_wrapper : std::false_type {};

template<typename T>
struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type{};

template <typename... T> struct is_vector : std::false_type {};
template <typename... T> struct is_linked_list : std::false_type {};
template <typename... T> struct is_forward_list : std::false_type {};

template <typename... T>
struct is_vector<std::vector<T...>> : std::true_type {};

template <typename... T>
struct is_container : std::integral_constant<bool, is_vector<T...>::value> {};

template<typename> struct is_tuple : std::false_type {};
template<typename... T> struct is_tuple<std::tuple<T...>> : std::true_type {};

#if 0
template <typename T> struct is_simple_task : std::false_type{};
template <typename T, typename P> struct is_priority_task : std::false_type{};

template <typename T> struct is_simple_task<simple_task<T>> : std::true_type{};
template <typename T, typename P> struct is_priority_task<priority_task<T,P>> : std::true_type{};
#endif

template<typename T>
struct FindTaskType_Impl;

template<typename T>
struct FindTaskType {
  typedef FindTaskType_Impl<std::remove_cvref_t<T>>::type type;
};

template<typename T>
struct FindTaskType_Impl<simple_task<T>> {
  typedef simple_task<T> type;
};

template<typename T, typename P>
struct FindTaskType_Impl<priority_task<T,P>> {
  typedef priority_task<T,P> type;
};

template<typename T>
struct FindTaskType_Impl<std::unique_ptr<T>> : FindTaskType_Impl<T> {};

template<typename T>
struct FindTaskType_Impl<std::vector<T>> : FindTaskType_Impl<T> {};

} // namespace traits
} // namespace thp

#endif // TRAITS_HPP
