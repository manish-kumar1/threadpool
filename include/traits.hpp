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
    : std::integral_constant<bool, is_unique_ptr_v<T> || is_shared_ptr_v<T>> {};

template <typename T> struct is_timepoint : std::false_type {};

template <typename T>
struct is_timepoint<std::chrono::time_point<T>> : std::true_type {};

template <typename... T> struct is_vector : std::false_type {};
template <typename... T> struct is_linked_list : std::false_type {};
template <typename... T> struct is_forward_list : std::false_type {};

template <typename... T>
struct is_vector<std::vector<T...>> : std::true_type {};

template <typename... T>
struct is_container : std::integral_constant<bool, is_vector<T...>::value> {};

// template<typename T>
// struct is_simple_task : std::false_type {};
// template<typename T>
// struct is_simple_task<::thp::simple_task<T>> : std::true_type {};

// template<typename T>
// struct is_priority_task : std::false_type {};
// template<typename... T>
// struct is_priority_task<::thp::priority_task<T...>> : std::true_type {};

// template<typename... T>
// struct is_task_type : std::integral_constant<bool, is_simple_task<T...> || is_priority_task<T...>> {};

} // namespace traits
} // namespace thp

#endif // TRAITS_HPP