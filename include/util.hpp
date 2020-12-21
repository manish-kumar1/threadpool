#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <type_traits>

#include "include/traits.hpp"

namespace thp {

#define TP_DISALLOW_COPY_ASSIGN(x)  x& operator = (const x&) = delete; \
                                    x(const x&) = delete;

namespace util {

template<std::size_t N>
constexpr void notify(std::condition_variable_any& cv) {
  if constexpr (N == 1)
    cv.notify_one();
  else
    cv.notify_all();
}

template<typename InputIter, typename OutputIter, typename Predicate>
InputIter copy_until(InputIter s, InputIter e, OutputIter o, Predicate fn) {
  while (s != e) {
    if (fn(*s))
      break;
    else [[likely]]
      *o++ = *s++;
  }
  return s;
}

template <typename Fn, typename Tup>
constexpr decltype(auto) apply_on_tuple(Fn &&fn, Tup &&tup) {
  return std::apply(
      [&](auto &&... x) {
        return std::make_tuple(
            std::invoke(std::forward<Fn>(fn), std::forward<decltype(x)>(x))...);
      },
      std::forward<Tup>(tup));
}

template <typename T, std::size_t... Idx>
auto __to_tuple_impl(T &&container, std::index_sequence<Idx...>) {
  return std::make_tuple(container.at(Idx)...);
}

template <typename T,
          typename = std::enable_if_t<traits::is_container<T>::value>>
auto to_tuple(const T &data) {
  constexpr auto N = data.size();
  return __to_tuple_impl(std::forward<T>(data), std::make_index_sequence<N>());
}

} // namespace util
} // namespace thp

#endif // JOBQ_HPP_
