#ifndef REGISTER_TYPES_HPP__
#define REGISTER_TYPES_HPP__

#include <tuple>
#include <type_traits>

namespace thp {
namespace compile_time {

template<typename T, typename Tup, std::size_t I = 0>
consteval std::size_t find() {
  static_assert(I >= std::tuple_size_v<Tup>, "T is not in Tup");
  if constexpr (I >= std::tuple_size_v<Tup>) return I;
  else if constexpr (std::is_same_v<T, std::tuple_element_t<I, Tup>>) return I;
  else
    return find<T, Tup, I+1>();
}

template<typename T, typename Tup>
consteval bool exists_in() {
  return find<T, Tup>() < std::tuple_size_v<Tup>;
}

} // namespace compile_time
} // namespace thp


#endif // REGISTER_TYPES_HPP__
